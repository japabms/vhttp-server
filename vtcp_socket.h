#ifndef V_TCP_SOCKET_H
#define V_TCP_SOCKET_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include "vutil.h"

typedef struct tcp_connection tcp_connection;
typedef struct tcp_server tcp_server;

struct tcp_server {
  struct sockaddr_in Socket;
  s32 FileDescriptor;

  tcp_connection* Connections;
  tcp_connection* WaitingConnections;
};

struct tcp_connection {
  void* Data; 

  struct sockaddr_in Socket;
  socklen_t Length;

  tcp_server* Server;
  
  s32 FileDescriptor;

  // NOTE: Waste of bytes down here.
  u8 Writable; // 0 == Done, 1 == To do;
  u8 Readable; // 0 == Done, 1 == To do;
  u8 Closed;
};

tcp_server* CreateTcpServer(const char* IpAddress, u16 Port, const u32 MaxConnections, const u32 WaitingConnections);

void TcpListen(const tcp_server* Server);

s64 Recv(s32 Socket, void* Buffer, u64 Length, s32 Flags);

s64 Send(s32 Socket, void* Buffer, u64 Length, s32 Flags);

void SetSocketNonBlocking(s32* FileDescriptor);
void SetSocketRecvTimeout(s32* FileDescriptor, u64 Secs, u64 MicroSecs);
void SetSocketKeepAlive(s32* FileDescriptor, s32 IdleSecs, s32 IntervalSecs, s32 Count);

#endif
