#include <asm-generic/errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "varray.h"
#include "vutil.h"
#include "vtcp_socket.h"

#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

void SetSocketNonBlocking(s32* FileDescriptor) {
  s32 Opts = fcntl(*FileDescriptor, F_GETFL, 0);
  fcntl(*FileDescriptor, F_SETFL, Opts | O_NONBLOCK);
}

void SetSocketRecvTimeout(s32* FileDescriptor, u64 Secs, u64 MicroSecs) {
  struct timeval Timeout;
  Timeout.tv_sec = Secs;
  Timeout.tv_usec = MicroSecs;

  setsockopt(*FileDescriptor,
             SOL_SOCKET,
             SO_RCVTIMEO,
             &Timeout,
             sizeof(Timeout));
}

/**
 * @brief      Recv wrapper
 *
 * @details    Recv wrapper that will retrieve data unti EWOULDBLOCK
 *
 * @param      Socket socket that recv will read
 * 
 * @param      Buffer buffer that recv will store the data
 * 
 * @param      Length the length of the buffer param
 * 
 * @param      Flags recv flags:
 *             MSG_PEEK, MSG_OOB, etc..
 *             
 * @return     Number of bytes recieved
 */
s64 NonblockRecv(s32 Socket, void* Buffer, u64 Length, s32 Flags) {
  if(Socket < 0) {
    // TODO: handle error invalid socket;
  }
  
  s64 BytesRecv = 0;
  while(true) {
    BytesRecv += recv(Socket, Buffer, Length, Flags);
    if(BytesRecv < 0) {
      s32 err = errno;
      if(err == EWOULDBLOCK) {
        break; // NOTE: There is no data avaliable.
      } else {
        VError("Recv");
      }
    }
  }
  
  return BytesRecv;
}

s64 Send(s32 Socket, void* Buffer, u64 Length, s32 Flags) {
  return send(Socket, Buffer, Length, Flags);
}

void SetSocketKeepAlive(s32* FileDescriptor, s32 IdleSecs, s32 IntervalSecs, s32 Count) {
  s32 Opt = 1;
  setsockopt(*FileDescriptor,
             SOL_SOCKET,
             SO_KEEPALIVE,
             &Opt,
             sizeof(Opt));

  setsockopt(*FileDescriptor,
             IPPROTO_TCP,
             TCP_KEEPIDLE,
             &IdleSecs,
             sizeof(s32));
  
  setsockopt(*FileDescriptor,
             IPPROTO_TCP,
             TCP_KEEPINTVL,
             &IntervalSecs,
             sizeof(s32));
  
  setsockopt(*FileDescriptor,
             IPPROTO_TCP,
             TCP_KEEPCNT,
             &Count,
             sizeof(s32));
}

tcp_server* CreateTcpServer(const char* IpAddress, u16 Port, const u32 MaxConnections, const u32 WaitingConnections) {
  tcp_server* Server = calloc(1, sizeof(tcp_server));
  
  Server->Connections = VArray_Init(tcp_connection, MaxConnections, false);
  Server->WaitingConnections = VArray_Init(tcp_connection, WaitingConnections, false);

  Server->FileDescriptor = socket(AF_INET,
                                  SOCK_STREAM,
                                  IPPROTO_TCP);

  if(Server->FileDescriptor < 0) {
    perror("Socket creation error");

    exit(EXIT_FAILURE);
  }

  int optval = 1;
  socklen_t optlen = sizeof(optval);
  if(setsockopt(Server->FileDescriptor,
                SOL_SOCKET,
                SO_REUSEADDR,
                &optval,
                optlen)) {
    perror("setsockopt()");
    close(Server->FileDescriptor);
    exit(EXIT_FAILURE);
  }
  
  if(setsockopt(Server->FileDescriptor,
                SOL_SOCKET,
                SO_REUSEPORT,
                &optval,
                optlen)) {
    perror("setsockopt()");
    close(Server->FileDescriptor);
    exit(EXIT_FAILURE);
  }

  SetSocketNonBlocking(&Server->Socket);

  memset(&Server->Socket, 0, sizeof(Server->Socket));

  Server->Socket.sin_family = AF_INET;
  Server->Socket.sin_port = htons(Port);
  
  // inet_addr could return -1 and this is a valid ip adress
  // TODO: use other alternatives later.
  Server->Socket.sin_addr.s_addr = inet_addr(IpAddress);

  return Server;
}

void TcpListen(const tcp_server* Server) {
  if(bind(Server->FileDescriptor, (struct sockaddr*) &Server->Socket, sizeof(Server->Socket)) < 0) {
    perror("Error while binding");
    exit(EXIT_FAILURE);
  }

  listen(Server->FileDescriptor, SOMAXCONN);
}
