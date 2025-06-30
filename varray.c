#include "varray.h"
#include "vutil.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define VARRAY_INIT_CAPACITY 16

void* _VArray_Init(u64 Capacity, u64 ItemSize, bool Growable) {
  void *Items = NULL;
  
  varray_header *Header = calloc(1, sizeof(varray_header) + (ItemSize * Capacity));
  assert(Header != NULL);

  Header->Capacity = Capacity;
  Header->Count = 0;
  Header->Growable = Growable;
  Items = Header + 1; // skips the header and begin the items

  return Items;
}

void* _VArray_Realloc(void* Array, uint64_t ItemSize) {
    if (!Array) return NULL;

    varray_header* Header = VArray_GetHeader(Array);
    u64 NewCapacity = Header->Capacity * 2;
    
    varray_header* NewHeader = realloc(Header, sizeof(varray_header) + NewCapacity * ItemSize);
    if (!NewHeader) return NULL;

    NewHeader->Capacity = NewCapacity;

    return (void*)(NewHeader + 1);
}
