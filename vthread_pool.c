#include "vthread_pool.h"
#include <bits/pthreadtypes.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

// Some prototypes.
static bool VThreadPool_IsAllThreadsInfinite(vthread_pool* Pool);
static void VThreadPool_AddThread(vthread_pool* Pool);

// ======================= Begin - VThreadList ==================================
inline static void VThreadList_Init(vthread_list* List, int Capacity) {
  List->Capacity = Capacity;
  List->Count = 0;
  
  List->Items = calloc(List->Capacity, sizeof(vthread));
}

inline static void VThreadList_Push(vthread_list* List, vthread* Thread) {
  if(List->Count >= List->Capacity) {
    List->Items = realloc(List->Items, sizeof(vthread) * (List->Capacity * 2));
    List->Capacity *= 2;
  }
  
  List->Items[List->Count] = *Thread;
  List->Count += 1;
}
// ======================= End - VThreadList ==================================

// ======================= Begin - VTaskList ==================================
inline static void VTask_Init(vtask* Task,
			      vtask_proc Procedure,
			      vtask_argument* Arg,
			      vtask_proc_attr Attr) {
  Task->Arg = Arg;
  Task->Procedure = Procedure;
  Task->Attr = Attr;
}

inline static void VTaskList_Init(vtask_list* List, int Capacity) {
  List->Capacity = Capacity;
  List->WriteCursor = List->Count = List->ReadCursor = 0;

  List->Items = calloc(List->Capacity, sizeof(vtask));
}

static void VTaskList_Enqueue(vthread_pool* Pool, vtask_list* List, vtask Task) {
  // [x] TODO: if all threads runs a procedure that have in it a inifinite loop
  // the enqueue will hang in this cond_wait, so i need to check if all thread
  // are running a inifinite procedure, if so, then create a new thread to handle
  // more procedures.
  // [] TODO: Make the threadpool add a thread every time the user adds a infinite
  // procedure, to make the first assign value num threads to be valid
  // [] TODO: Set a max number thread that can be created.
  pthread_mutex_lock(Pool->Lock);
  if(VThreadPool_IsAllThreadsInfinite(Pool)) {
    VThreadPool_AddThread(Pool);
  }


  while(List->Count == List->Capacity) {
    pthread_cond_wait(Pool->ThreadDone, Pool->Lock);
  }


  
  List->Items[List->WriteCursor] = Task;
  List->WriteCursor = (List->WriteCursor + 1) % List->Capacity;

  List->Count += 1;
  pthread_mutex_unlock(Pool->Lock);
}

inline static vtask VTaskList_Dequeue(vtask_list* List) {
  if(List->Count == 0) {
    return (vtask){0};
  }
  
  vtask Task = List->Items[List->ReadCursor];
  List->ReadCursor = (List->ReadCursor + 1) % List->Capacity;
  List->Count -= 1;

  
  return Task;
}
// ======================= End - VTaskList ==================================


// ======================= Begin - VThread ==================================
inline static void VThread_Init(vthread* Thread, vthread_pool* Pool) {
  Thread->Id = 0;
  Thread->Pool = Pool;
  Thread->State = THREAD_IDLE;
}

static vtask_argument* VThread_Main(vtask_argument* Arg) {
  vthread* Thread = (vthread*)Arg;
  pthread_detach(Thread->Id);
  vthread_pool* Pool = Thread->Pool;
  vtask Task;

  pthread_mutex_lock(Pool->Lock);
  Pool->ThreadCount += 1;
  pthread_mutex_unlock(Pool->Lock);

  #if _VDEBUG_
  printf("Thread information:\n");
  printf("Id: %lu\n", Thread->Id);
  printf("Pool: %x\n", Thread->Pool);
  #endif
  
  for(;;) {
    // Taking a task from queue. ---------------
    pthread_mutex_lock(Pool->Lock);

    while(Pool->TaskList->Count == 0) {
      Thread->State = THREAD_IDLE;
      pthread_cond_wait(Pool->TaskAvaliable, Pool->Lock);
      if(Pool->Terminate) {
	Pool->ThreadCount -= 1;
	pthread_mutex_unlock(Pool->Lock);
	
	pthread_exit(NULL);
      }
    }

    // Get task.
    Task = VTaskList_Dequeue(Pool->TaskList);
    pthread_mutex_unlock(Pool->Lock);
    // -----------------------------------------------
    
    if(Task.Procedure != 0) {
      Thread->State = THREAD_RUNNING;
      
      if(Task.Attr == PROC_INFINITY) {
	Thread->State = THREAD_INFINITE;
	pthread_mutex_lock(Pool->Lock);
	Pool->InfiniteThreadsWorking += 1;
	pthread_mutex_unlock(Pool->Lock);
      } else if(Task.Attr == PROC_DEFAULT) {
	// printf("[Thread %lu] else kk %d\n", Thread->Id, Task.Attr);
	pthread_mutex_lock(Pool->Lock);
	Pool->NormalThreadsWorking += 1;
	pthread_mutex_unlock(Pool->Lock);
      }
      
      // Run the task.
      Task.Procedure(Task.Arg);

      // NOTE: Will never reach if its a inifinite procedure.
      pthread_mutex_lock(Pool->Lock);
      pthread_cond_signal(Pool->ThreadDone);
      
      Thread->State = THREAD_IDLE;
      Pool->NormalThreadsWorking -= 1;
      
      pthread_mutex_unlock(Pool->Lock);
    }

    pthread_mutex_lock(Pool->Lock);
    if(Pool->NormalThreadsWorking == 0) {
      pthread_cond_signal(Pool->AllThreadsDone);
    }
    pthread_mutex_unlock(Pool->Lock);
  }
  
  return NULL;
}
// ======================= End - VThread ==================================

// ======================= Begin - VThreadPool ============================
vthread_pool* VThreadPool_Init(uint32_t NumThreads) {
  vthread_pool* Result = calloc(1, sizeof(vthread_pool));
  Result->Threads = calloc(1, sizeof(vthread_list));
  Result->TaskList = calloc(1, sizeof(vtask_list));
  Result->Lock = calloc(1, sizeof(pthread_mutex_t));
  Result->TaskAvaliable = calloc(1, sizeof(pthread_cond_t));
  Result->AllThreadsDone = calloc(1, sizeof(pthread_cond_t));
  Result->ThreadDone = calloc(1, sizeof(pthread_cond_t));

  pthread_mutex_init(Result->Lock, NULL);
  
  pthread_cond_init(Result->TaskAvaliable, NULL);
  pthread_cond_init(Result->AllThreadsDone, NULL);
  pthread_cond_init(Result->ThreadDone, NULL);
  
  VTaskList_Init(Result->TaskList, NumThreads);
  VThreadList_Init(Result->Threads, NumThreads);

  vthread Threads[NumThreads];
  
  for(u64 I = 0; I < NumThreads; I++) {
    VThread_Init(&Threads[I], Result);
    VThreadList_Push(Result->Threads, &Threads[I]);
    pthread_create(&Result->Threads->Items[I].Id,
		   NULL,
		   VThread_Main,
		   &Result->Threads->Items[I]);
  }

  return Result;
}

void VThreadPool_AddTask(vthread_pool* Pool, vtask_proc Procedure, vtask_argument* Arg, vtask_proc_attr Attr) {
  vtask Task;

  if(Attr == 0) {
    Attr = PROC_DEFAULT;
  }
  
  VTask_Init(&Task, Procedure, Arg, Attr);
  VTaskList_Enqueue(Pool, Pool->TaskList, Task);

  pthread_cond_signal(Pool->TaskAvaliable);
}

static bool VThreadPool_IsAnyThreadInfinite(vthread_pool* Pool) {
  for(int I = 0; I < Pool->Threads->Count; I++) {
    if(Pool->Threads->Items[I].State == THREAD_INFINITE) {
      return true;
    }
  }

  return false;
}

static bool VThreadPool_IsAllThreadsInfinite(vthread_pool* Pool) {
  uint32_t Count = 0;
  for(int I = 0; I < Pool->Threads->Count; I++) {
    if(Pool->Threads->Items[I].State == THREAD_INFINITE) {
      Count += 1;
    }
  }
  
  if(Count == Pool->Threads->Count) {
    return true;
  }

  return false;
}

static void VThreadPool_AddThread(vthread_pool* Pool) {
  vthread Thread = {0};
  VThread_Init(&Thread, Pool);
  VThreadList_Push(Pool->Threads, &Thread);
  pthread_create(&Pool->Threads->Items[Pool->Threads->Count - 1].Id,
		 NULL,
		 VThread_Main,
		 &Pool->Threads->Items[Pool->Threads->Count - 1]);
}

void VThreadPool_Wait(vthread_pool* Pool) {
  pthread_mutex_lock(Pool->Lock);
  while(Pool->NormalThreadsWorking > 0 || Pool->TaskList->Count > 0) {
    printf("Waiting...\n");
    pthread_cond_wait(Pool->AllThreadsDone, Pool->Lock);
  }
  printf("Stop Waiting...\n");
  pthread_mutex_unlock(Pool->Lock);
}

void VThreadPool_Destroy(vthread_pool* Pool) {
  pthread_mutex_lock(Pool->Lock);
  Pool->Terminate = 1;
  pthread_mutex_unlock(Pool->Lock);
  
  for(int I = 0; I < Pool->Threads->Count; I++) {
    vthread* Thread = &Pool->Threads->Items[I];
    if(Thread->State == THREAD_INFINITE) {
      Thread->State = THREAD_DEAD;
      pthread_cancel(Pool->Threads->Items[I].Id);
    }
    usleep(5000);
  }

  // TODO: Wait for not inifine threads to be done
  
  /* while(Pool->ThreadCount > 0) { */
  /*   pthread_cond_broadcast(Pool->TaskAvaliable); */
  /*   usleep(5000); */
  /* } */
  
  pthread_mutex_destroy(Pool->Lock);  
  pthread_cond_destroy(Pool->TaskAvaliable);
  pthread_cond_destroy(Pool->AllThreadsDone);
  pthread_cond_destroy(Pool->ThreadDone);
  free(Pool->Threads->Items);
  free(Pool->TaskList->Items);
  free(Pool->Threads);
  free(Pool->TaskList);
  free(Pool->Lock);
  free(Pool->TaskAvaliable);
  free(Pool->AllThreadsDone);
  free(Pool->ThreadDone);
  free(Pool);
}
// ======================= End - VThreadPool ============================
