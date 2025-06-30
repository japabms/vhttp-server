#ifndef _V_THREAD_POOL_H
#define _V_THREAD_POOL_H
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "vutil.h"

typedef struct vtask_list vtask_list;
typedef enum vthread_state vthread_state;

typedef void vtask_argument;
typedef void* (*vtask_proc)(vtask_argument* Argument);

typedef struct vtask vtask;
typedef struct vthread_pool vthread_pool;
typedef struct vthread vthread;
typedef struct vthread_list vthread_list;
typedef enum vtask_proc_attr vtask_proc_attr;

enum vthread_state {
  THREAD_IDLE,
  THREAD_RUNNING,
  THREAD_INFINITE,
  THREAD_DEAD,
};

enum vtask_proc_attr {
  PROC_DEFAULT,
  PROC_INFINITY,
};

struct vtask {
  vtask_proc Procedure;
  vtask_argument* Arg;
  vtask_proc_attr Attr;
};

// TODO(victor): Make a priority queue?
// NOTE(victor): FIFO
struct vtask_list {
  vtask* Items;
  u64 Count;
  u64 WriteCursor;
  u64 ReadCursor;
  u64 Capacity;
};

struct vthread {
  pthread_t Id;
  vthread_pool* Pool;  // Reference for the pool that i'm part
  vthread_state State;
};

struct vthread_list {
  vthread* Items;
  u64 Count;
  u64 Capacity;
};

struct vthread_pool {
  vthread_list* Threads;
  vtask_list* TaskList;

  volatile uint32_t InfiniteThreadsWorking;
  volatile uint32_t NormalThreadsWorking;
  volatile uint32_t ThreadCount; // dont know if its needed
  
  
  pthread_cond_t* AllThreadsDone;
  pthread_cond_t* ThreadDone;
  
  pthread_cond_t* TaskAvaliable;
  
  pthread_mutex_t* Lock;

  volatile uint32_t Terminate;
};

vthread_pool* VThreadPool_Init(uint32_t NumThreads);
void VThreadPool_Wait(vthread_pool* Pool);
void VThreadPool_AddTask(vthread_pool* Pool, vtask_proc Procedure, vtask_argument* Arg, enum vtask_proc_attr Attr);
void VThreadPool_Destroy(vthread_pool* Pool);

#endif
