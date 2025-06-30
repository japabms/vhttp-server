#include "vepoll.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

vepoll* VEpoll_Create() {
  vepoll* Epoll = calloc(1, sizeof(vepoll));
  
  if((Epoll->FileDescriptor = epoll_create1(0)) == -1) {
    perror("epoll_create1");
    return NULL;
  }
  
  return Epoll;
}

s32 VEpoll_AddFd(vepoll* Epoll, u32 Options, s32 FileDescriptor, u64 Idx) {
  struct epoll_event Event;
  Event.events = Options;
  Event.data.u64 = Idx;

  if((epoll_ctl(Epoll->FileDescriptor, EPOLL_CTL_ADD, FileDescriptor, &Event)) == -1) {
    s32 err = errno;
    fprintf(stderr, "[ERRNO %d] ", err);
    perror("epoll_ctl_add");
  }
  return 0;
}

s32 VEpoll_RemoveFd(vepoll* Epoll, s32 FileDescriptor) {
  if(epoll_ctl(Epoll->FileDescriptor, EPOLL_CTL_DEL, FileDescriptor, NULL) == - 1) {
    s32 err = errno;
    fprintf(stderr, "[ERRNO %d] ", err);
    perror("epoll_ctl_del");
  }

  return 0;
}

