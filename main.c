#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "varray.h"
#include "vtcp_socket.h"
#include "vthread_pool.h"
#include "http_request.h"
#include "vmime.h"
#include "vstring.h"
#include "vfile.h"
#include "vepoll.h"

// TODO: Remove this temporary prototype
void ProcessEvents(vepoll *Epoll, vthread_pool *TPool, tcp_server* Server);

static char* AppRoot = "/home/victor";
static u64 LastResponseLength = 0;
#define RESPONSE_MAX_LEN 1024*1024 // NOTE: 1 Mb of response
static char* ResponseMalloced;

static const char G_ForbiddenResponseFmt[] =
  "HTTP/1.1 403 FORBIDDEN\r\n"
  "Server: vserve\r\n"
  "Content-Length: %d\r\n"
  "Content-Type: %s\r\n"
  "Connection: close\r\n\r\n"
  "%s";

static const char G_NotFoundResponseFmt[] =
  "HTTP/1.1 404 NOT FOUND\r\n"
  "Server: vserve\r\n"
  "Content-Length: 0\r\n"
  "Content-Type: %s\r\n"
  "Connection: close\r\n\r\n";

static const char G_BadRequestResponseFmt[] =
  "HTTP/1.1 400 BAD REQUEST\r\n"
  "Server: vserve\r\n"
  "Content-Length: 0\r\n"
  "Content-Type: %s\r\n"
  "Connection: close\r\n\r\n";

static const char G_ServiceUnavailableResponseFmt[] =
  "HTTP/1.1 503 SERVICE UNAVAILABLE\r\n"
  "Server: vserve\r\n"
  "Content-Length: 0\r\n"
  "Content-Type: plain/text\r\n"
  "Connection: close\r\n\r\n";

static const char G_OkResponseFmt[] =
  "HTTP/1.1 200 OK\r\n"
  "Server: vserve\r\n"
  "Connection: Keep-Alive\r\n"
  "Content-Length: %d\r\n"
  "Content-Type: %s\r\n\r\n";

void LogClientInfo(tcp_connection *Client) {
  char buff[4096] = {0};
  struct sockaddr* Info = (struct sockaddr*)&Client->Socket;
  switch(Info->sa_family) {
    case AF_INET:
      {
        inet_ntop(AF_INET,
                  &Client->Socket.sin_addr,
                  buff,
                  sizeof(buff));
      } break;
    case AF_INET6:
      {
        inet_ntop(AF_INET6,
                  &Client->Socket.sin_addr,
                  buff,
                  sizeof(buff));	
      } break;
  }
  
  printf("[INFO]: Accepting connection from %s\n", buff);
}

typedef struct sendfile_args {
  s32 FileDescriptor;
  vfile File;
} sendfile_args;

// FIXME: ...
void* ThreadSendfile(void* Args) {
  sendfile_args SendfileArgs = *(sendfile_args*) Args;
  s64 Sended = 0;
  while(Sended < SendfileArgs.File.Info.st_size) {
    s64 Ret = sendfile(SendfileArgs.FileDescriptor,
             SendfileArgs.File.FileDescriptor,
             &Sended,
             SendfileArgs.File.Info.st_size - Sended);
    
    if(Ret == -1) {
      int err = errno;

      if(err == EPIPE || err == EBADF || err == ECONNRESET || err == ESPIPE) {
        close(SendfileArgs.FileDescriptor);
        break;
      } else if(err == EAGAIN || err == EWOULDBLOCK) {
        continue;
      }
      
      printf("%s %d\n", strerror(err), err);
    }
  }

  VFile_Close(&SendfileArgs.File);
  return NULL;
}

// TODO: Add connection to the function signature
void EpollHttpWorker(vepoll* Epoll, vthread_pool* TPool, s32 FileDescriptor, tcp_connection* Conn) {
  vstring* StringPool = VString_InitWithCapacity(NULL, 4092); // NOTE: Memory allocation
  vhttp_request Request = {0};
  s64 Ret;
  s32 Tries = 0;

  VHttpRequest_Init(&Request); // NOTE: Memory allocation
  
  // TODO: Check for erros;  
  for(;;) {
    // NOTE: DO a MSG_PEEK, prepare the request, if the request has a high
    // content length, do another recv.
    if ((Ret = recv(FileDescriptor, StringPool->Data, 4092, MSG_PEEK)) < 0) {
      s32 err = errno;
      if(err == EWOULDBLOCK || err == EAGAIN) {
        break;
      } else if(err == ETIMEDOUT || err == ECONNRESET) {
        printf("Socket is DEAD.\n");
        goto cleanup;
      } else {
        perror("recv");
        printf("%d\n", FileDescriptor);
        goto cleanup;
      }
    } else if(Ret == 0) {
      printf("Connection closed by peer %d\n", FileDescriptor);
      goto cleanup; // NOTE: Connection closed by peer
    }

    StringPool->Length = (u64) Ret;

    // NOTE: Check if found \r\n\r\n
    u64 RequestSize = VString_FindSubstring(StringPool, "\r\n\r\n");
    if(RequestSize > 0) {
      char DiscardBuffer[RequestSize];
      Ret = recv(FileDescriptor, DiscardBuffer, RequestSize, 0);
      if(Ret < 0) {
        s32 err = errno;
        if(err == EWOULDBLOCK || err == EAGAIN) {
          break;
        } else if(err == ETIMEDOUT || err == ECONNRESET) {
          printf("Socket is DEAD.\n");
          goto cleanup;
        } else {
          perror("recv");
          printf("%d\n", FileDescriptor);
          goto cleanup;
        }
      }

      bool ParseOk = VHttpRequest_Parse(&Request, StringPool);
      if(!ParseOk) {
        s32 Size = snprintf(ResponseMalloced, RESPONSE_MAX_LEN, G_BadRequestResponseFmt, "text/plain");
        send(Conn->FileDescriptor, ResponseMalloced, Size, MSG_NOSIGNAL);
        memset(ResponseMalloced, 0, Size);
        goto cleanup;
      }
      
      // NOTE: if have a body, get it with RECV
      // TODO: Check for Expect: 100-continue
      if(Request.ContentLength != 0) {
        VString_Resize(Request.Body, Request.ContentLength + 1);
        // FIXME: Do a loop to guarantee the retrieve of the body
        recv(FileDescriptor, Request.Body, Request.ContentLength, 0);
      }
    } else {
      // TODO: make this better?
      Tries += 1;
        if(Tries == 5) {
          s32 Size = snprintf(ResponseMalloced, RESPONSE_MAX_LEN, G_BadRequestResponseFmt, "text/plain");
          send(Conn->FileDescriptor, ResponseMalloced, Size, MSG_NOSIGNAL);
          memset(ResponseMalloced, 0, Size);
          goto cleanup;          
        }
      // TODO: Do realloc and call recv again.
    }
  }
  
  if(StringPool->Length == 0) {
    goto cleanup;
  }
    
  memset(StringPool->Data, 0, StringPool->Length); StringPool->Length = 0;
  
  vstring* FilePath = VString_Init(NULL); // NOTE: Memory allocation
  VString_ConcatCString(FilePath, AppRoot);
  VString_Concat(FilePath, Request.Url);
  
  char* Extension = strrchr(FilePath->Data, '.');
  mime_mapping Mime = GetMimeType(Extension);
  
  vfile File = {0};  
  if(!Mime.UseSendfile) {
    if(VString_ReadEntireFile(StringPool, FilePath->Data) == 0) {
      snprintf(ResponseMalloced, RESPONSE_MAX_LEN,
               G_NotFoundResponseFmt, Mime.MimeType);
    } else {
      snprintf(ResponseMalloced, RESPONSE_MAX_LEN,
               G_OkResponseFmt, StringPool->Length, Mime.MimeType);
      
      strncat(ResponseMalloced, StringPool->Data, StringPool->Length);
    }
  } else {
    // TODO: check for file not found.
    File = VFile_Open(FilePath->Data, O_RDONLY);
    if(File.FileDescriptor < 0) {
      strncpy(ResponseMalloced, G_NotFoundResponseFmt, sizeof(G_NotFoundResponseFmt));
      Mime.UseSendfile = 0;
    } else {
      switch(VFile_GetType(&File)) {
        case VFILE_REGULAR:  {
          snprintf(ResponseMalloced, RESPONSE_MAX_LEN,
                   G_OkResponseFmt, File.Info.st_size, Mime.MimeType);
          break;
        } 
        default:  {
          const char* html = "<html><body><h1>Forbidden</h1></body></html>";
          
          sprintf(ResponseMalloced,
                  G_ForbiddenResponseFmt,
                  45,
                  "text/html; charset=utf-8",
                  html);
          
          Mime.UseSendfile = 0;
          break;
        }
      }
    }
  }

  u64 ResponseLen = strlen(ResponseMalloced);
  
  u64 TotalSent = 0;
  while (TotalSent < ResponseLen) {
    Ret = send(FileDescriptor,
               ResponseMalloced + TotalSent,
               ResponseLen - TotalSent,
               0);
    
    if (Ret < 0) {
      s32 err = errno;
      if (err == EAGAIN || err == EWOULDBLOCK) {
        break;
      } else {
        perror("send error");
        goto cleanup;
      }
    }
    TotalSent += Ret;
  }

  if(Mime.UseSendfile) {
    sendfile_args Args = {
      .File = File,
      .FileDescriptor = FileDescriptor,
    };

    #if 1
    s64 Sended = 0;
    while(Sended < Args.File.Info.st_size) {
      sendfile(Args.FileDescriptor,
               Args.File.FileDescriptor,
               &Sended,
               Args.File.Info.st_size - Sended);
    }
    
    #else
    // FIXME: The thread pool still sends the file if the socket was closed so i
    // need to make sure that the socket wait until all data was send to close
    VThreadPool_AddTask(TPool, ThreadSendfile, &Args, PROC_DEFAULT);    
    #endif
  }

  // Cleaning the last response
  memset(ResponseMalloced, 0,
         LastResponseLength == 0
         ? ResponseLen
         : LastResponseLength);
  
  LastResponseLength = ResponseLen;
  VString_Free(StringPool);
  VString_Free(FilePath);
  VHttpRequest_Destroy(&Request);
  return;

  // Remove this goto later;
 cleanup:
  printf("[INFO]: cleanup\n");
  Conn->Closed = 1;
  close(Conn->Closed);
  VString_Free(StringPool);
  VHttpRequest_Destroy(&Request);
  return;
}

void DEBUG_LogConnection(tcp_connection *c) {
  printf("tcp_connection { id: %lu fd:%d closed:%d r&w: %d% d }\n", c->Id, c->FileDescriptor, c->Closed, c->Readable, c->Writable);
}

int main(void) {
  ResponseMalloced = calloc(RESPONSE_MAX_LEN, sizeof(char));

  vthread_pool* TPool = VThreadPool_Init(4);
  
  tcp_server* TcpServer = CreateTcpServer("0.0.0.0",
                                          8080,
                                          SOMAXCONN,
                                          10); // TODO: remove 'WaitingConnections' from this function.
  TcpListen(TcpServer);
  
  vepoll* Epoll =   VEpoll_Create();
  
  VEpoll_AddFd(Epoll,
               EPOLLIN,
               TcpServer->FileDescriptor, 9999);
  
  // default epoll loop founded on epoll man page.
  while(true) {
    // TODO: Create VEpoll_Wait
    Epoll->EventsRecieved = epoll_wait(Epoll->FileDescriptor,
                                       Epoll->Events,
                                       MAX_EPOLL_EVENTS,
                                       -1);

    // fprintf(stdout, "[INFO]: Events recieved: %ld\n", Epoll->EventsRecieved);
    if(Epoll->EventsRecieved == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }
    
    for(int Idx = 0; Idx < Epoll->EventsRecieved; Idx++) {
      u32 EventType = Epoll->Events[Idx].events;
      u64 EpollIdx = Epoll->Events[Idx].data.u64;
      tcp_connection* Conn = &TcpServer->Connections[EpollIdx];

      // NOTE: Cleaning closed connections.
      if(EventType & (EPOLLHUP|EPOLLRDHUP)) {
        fprintf(stdout, "[INFO]: Connection closed [%lu/%ld]:%d\n", Conn->Id, EpollIdx, Conn->FileDescriptor);
        if(!Conn->Closed) {
          Conn->Closed = 1;
          VEpoll_RemoveFd(Epoll, Conn->FileDescriptor);
          close(Conn->FileDescriptor); 
        }
      }

      if(EventType & EPOLLIN) {
        // NOTE: Server recieved a in event.
        if(Epoll->Events[Idx].data.u64 == 9999) {
          tcp_connection Client = {0};

          // NOTE: Accept a connection.
          Client.Length = sizeof(Client.Socket);
          // TODO: Dont accept when the connection limit reach
          Client.FileDescriptor = accept(TcpServer->FileDescriptor,
                                         (struct sockaddr*) &Client.Socket,
                                         &Client.Length);
          if(Client.FileDescriptor == -1) {
            perror("[ERROR]: accept");
            continue; 
          }
          
          Client.Server = TcpServer;

          // SetSocketRecvTimeout(&Client.FileDescriptor, 0, 500);
          SetSocketNonBlocking(Client.FileDescriptor);
          SetSocketKeepAlive(&Client.FileDescriptor,
                             30,
                             5,
                             5); // TODO: set this when readed http request.

          u64 ConnCount = VArray_Count(TcpServer->Connections);
          u64 ConnCapacity = VArray_Capacity(TcpServer->Connections);

          // FIXME: THAT SHIT IS CONVOLUTED AS FUCK. SIMPLIFY THIS SHIT.          
          if(!(ConnCount == ConnCapacity)) {
            // NOTE: Add to epoll first, to VArray_Count return the correct idx.
            u64 NewConnIdx = VArray_Count(TcpServer->Connections);
            // fprintf(stdout, "[INFO]: Setting up new connection: [%d]:%d\n", NewConnIdx, Client.FileDescriptor);
            Client.Id = NewConnIdx;

            s32 FoundReusable = 0;
            for(int i = 0; i < ConnCount; i++) {
              tcp_connection* Conn2 = &TcpServer->Connections[i];
              if(Conn2->Closed) {
                // fprintf(stdout, "[INFO]: Reusing connection %lu\n", Conn2->Id);
                Client.Id = Conn2->Id;
                memcpy(Conn2, &Client, sizeof(Client));
                VEpoll_AddFd(Epoll,
                             EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP,
                             Client.FileDescriptor,
                             Conn2->Id);
                FoundReusable = 1;
                break;
              }
            }
            // NOTE: Adding new connection
            if(!FoundReusable) {
              VEpoll_AddFd(Epoll,
                           EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP,
                           Client.FileDescriptor,
                           NewConnIdx);
              VArray_Push(TcpServer->Connections, Client);
            }
            
          } else {
            // NOTE: Reusing connection
            int Found = 0;
            for(u32 i = 0; i < ConnCapacity; i++) {
              tcp_connection* Conn2 = &TcpServer->Connections[i];
              if(Conn2->Closed && Conn2->Closed) {
                // fprintf(stdout, "[INFO]: Reusing connection %lu\n", Conn2->Id);
                memcpy(Conn2, &Client, sizeof(Client));
                VEpoll_AddFd(Epoll,
                             EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP,
                             Conn2->FileDescriptor,
                             Conn2->Id);
                Found = 1;
                break;
              }
            }

            // NOTE: This only handles the case when the user limited connections ocurr.
            // TODO: Handle the case that the OS cant open fd's anymore.
            if(!Found) {
              printf("[INFO]: There is no closed connection available\n");
              // NOTE: Sending HTTP 503 service unvailable if there is no avaliable connection.
              send(Client.FileDescriptor,
                   G_ServiceUnavailableResponseFmt,
                   sizeof(G_ServiceUnavailableResponseFmt),
                   MSG_NOSIGNAL);
              
              close(Client.FileDescriptor);
            }
          }

          int size;
          socklen_t len = sizeof(size);
          // NOTE: Gets the kernel send buffer size
          getsockopt(Client.FileDescriptor,
                     SOL_SOCKET,
                     SO_SNDBUF,
                     &size,
                     &len);
        } else {
          // NOTE: Other socket than the server is ready to be read.
          Conn->Readable = 1;
        } 
      }

      // NOTE: Event ready to write.
      if(EventType & EPOLLOUT) {
        Conn->Writable = 1;
      }
    }
    
    //    log_connections(TcpServer->Connections, 0);
    ProcessEvents(Epoll, TPool, TcpServer);
  }
  
  return 0;
}

void ProcessEvents(vepoll *Epoll, vthread_pool *TPool, tcp_server* Server) {
  u64 ConnectionsCount = VArray_Count(Server->Connections);
  
  for(u32 Idx = 0; Idx < ConnectionsCount; Idx++) {
    tcp_connection* Conn = &Server->Connections[Idx];
    // debug_log_connection(Conn);
    if(Conn->Closed) {
      continue;
    }
    
    if(Conn->Readable) {
      EpollHttpWorker(Epoll, TPool, Conn->FileDescriptor, Conn);
      Conn->Readable = 0;
    }
    
    if(Conn->Writable) {
      Conn->Writable = 0;
    }
  }
}
