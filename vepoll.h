#ifndef _V_EPOLL_H_
#define _V_EPOLL_H_

#include <sys/epoll.h>
#include "vutil.h"

#define MAX_EPOLL_EVENTS 128
typedef struct epoll_handler {
  struct epoll_event Events[MAX_EPOLL_EVENTS];
  s32 FileDescriptor;
  s64 EventsRecieved;
} vepoll;

vepoll* VEpoll_Create(void);
s32 VEpoll_AddFd(vepoll* Epoll, u32 Options, s32 FileDescriptor, u64 Idx);
s32 VEpoll_RemoveFd(vepoll* Epoll, s32 FileDescriptor);

#endif
