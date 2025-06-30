#ifndef _V_ARRAY_H_
#define _V_ARRAY_H_

#include "vutil.h"
#include <stdbool.h>

typedef struct varray_header varray_header;
struct varray_header {
  u64 Count;
  u64 Capacity;

  bool Growable;
  
  u64 padding;
};

void* _VArray_Init(u64 Capacity, u64 ItemSize, bool Growable);
void* _VArray_Realloc(void* Array, u64 ItemSize);

#define VArray_Init(Type, Capacity, Growable) (Type*)_VArray_Init(Capacity, sizeof(Type), Growable)

// Don't use this.
#define VArray_GetHeader(Array) (((varray_header*)Array) - 1)

#define VArray_Push(Array, Item)                                        \
  do {                                                                  \
    varray_header* Header = VArray_GetHeader(Array);                    \
    if(Header->Count >= Header->Capacity && Header->Growable) {         \
      VArray_Realloc(Array);                                            \
      Header = VArray_GetHeader(Array);                                 \
    } else if(Header->Count >= Header->Capacity && !Header->Growable) { \
      printf("[WARNING] Array is full and its not growable.\n");        \
      break;                                                            \
    }                                                                   \
    Array[Header->Count] = Item;                                        \
    Header->Count += 1;                                                 \
  } while(0);                                                           \


#define VArray_Realloc(Array)                       \
  do {                                              \
    Array = _VArray_Realloc(Array, sizeof(*Array)); \
    assert(Array != NULL);                          \
  } while(0);                                       \


#define VArray_Insert(Array, Item, Idx)                                 \
  do {                                                                  \
    assert(Array != NULL);                                              \
    varray_header* Header = VArray_GetHeader(Array);                    \
    assert(Idx <= Header->Count);                                       \
    if(Header->Count >= Header->Capacity && Header->Growable) {         \
      VArray_Realloc(Array);                                            \
      Header = VArray_GetHeader(Array);                                 \
    } else if(Header->Count >= Header->Capacity && !Header->Growable) { \
      printf("[WARNING] Array is full and its not growable.\n");        \
      break;                                                            \
    }                                                                   \
                                                                        \
    memmove(&Array[Idx+1], &Array[Idx],                                 \
            sizeof(*Array)*(Header->Count - Idx));                      \
                                                                        \
    Array[Idx] = Item;                                                  \
    Header->Count += 1;                                                 \
  } while(0);                                                           \
  

#define VArray_Remove(Array, Idx)                                       \
  do {                                                                  \
    assert(Array != NULL);                                              \
    varray_header* Header = VArray_GetHeader(Array);                    \
    if(Idx == Header->Count - 1) {                                      \
      Header->Count--;                                                  \
      break;                                                            \
    }                                                                   \
                                                                        \
    if(Idx < Header->Count) {                                           \
      memmove(&Array[Idx], &Array[Idx+1],                               \
              sizeof(*Array) * (Header->Count - Idx - 1));              \
      Header->Count--;                                                  \
    }                                                                   \
  } while(0);                                                           \


#define VArray_UnorderedRemove(Array, Idx)            \
  do {                                                \
    assert(Array != NULL);                            \
    varray_header* Header = VArray_GetHeader(Array);  \
    if(Idx == Header->Count - 1) {                    \
      Header->Count--;                                \
      break;                                          \
    }                                                 \
                                                      \
    if(Idx < Header->Count) {                         \
      Array[Idx] = Array[Header->Count - 1];          \
      Header->Count--;                                \
    }                                                 \
                                                      \
                                                      \
  } while(0);                                         \

#define VArray_Count(Array) VArray_GetHeader(Array)->Count
#define VArray_Capacity(Array) VArray_GetHeader(Array)->Capacity
#define VArray_Destroy(Array) free(VArray_GetHeader(Array))

#endif
