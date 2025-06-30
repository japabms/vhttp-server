#include "vhash_table.h"
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define INIT_V_HASH_TABLE_CAPACITY 16

// TODO:
// - Add double hashing

struct vhash_table{
  vhash_item* Items;
  u64 Count;
  u64 Capacity;
};

vhash_iterator VHashTable_Iterator(vhash_table* Table) {
  vhash_iterator Result = {0};
  Result.Cursor = 0;
  Result.Count = 0;
  Result.Table = Table;
  
  return Result;
}

vhash_table* VHashTable_Init(u64 Capacity) {
  vhash_table* Result = calloc(1, sizeof(vhash_table));
  if(Result == NULL) return NULL;
  
  if(Capacity == 0) {
    Capacity = INIT_V_HASH_TABLE_CAPACITY;
  }

  Result->Items = calloc(Capacity, sizeof(vhash_item));
  Result->Count = 0;
  Result->Capacity = Capacity;
  
  return Result;
}

void VHashTable_Free(vhash_table *Table) {
  for(s32 Idx = 0; Idx < Table->Count; Idx++) {
    if(Table->Items[Idx].Key != NULL) {
      free(Table->Items[Idx].Key);
      if(Table->Items[Idx].Value != NULL) {
        free(Table->Items[Idx].Value);
      }
    }
  }
  
  free(Table->Items);
  free(Table);
}

// djb2 hash function
static u64 VHashTable_Hash(vhash_key* Key) {
  unsigned long Hash = 5381;
  int c;
  while ((c = *Key++))
    Hash = ((Hash << 5) + Hash) + c; // hash * 33 + c
  return Hash;
}

static bool VHashTable_ItemIsEmpty(vhash_table* Table, u64 Index) {
  return Table->Items[Index].Key == NULL;
}

static void VHashTable_Rehash(vhash_table* Table) {
  u64 OldCapacity = Table->Capacity;
  vhash_item* OldItems = Table->Items;


  Table->Capacity *= 2;
  Table->Count = 0;
  Table->Items = calloc(Table->Capacity, sizeof(vhash_item));

  for(int i = 0; i < OldCapacity; i++) {
    vhash_item Item = OldItems[i];
    
    u64 NewIndex = Item.Hash % Table->Capacity;
    while(!VHashTable_ItemIsEmpty(Table, NewIndex)) {
      // Linear Probing
      NewIndex = (NewIndex+1) % Table->Capacity;      
    }

    Table->Items[NewIndex] = Item;
  }
  free(OldItems);
}

bool VHashTable_Insert(vhash_table* Table, vhash_key* Key, vhash_value* Value, u64 SizeofValue) {
  if(Table->Count >= Table->Capacity) {
    // TODO(victor): Rehash
    VHashTable_Rehash(Table);
  }
  vhash_item Item;
  
  vhash_value* NewValue = calloc(1, SizeofValue);
  memcpy(NewValue, Value, SizeofValue);
  
  Item.Key = (uchar*) strdup(Key);
  Item.Value = NewValue;

  u64 Hash = VHashTable_Hash(Key);
  Item.Hash = Hash;
  
  u64 Index = Hash % Table->Capacity;
  while(!VHashTable_ItemIsEmpty(Table, Index)) {
    if(strcmp(Table->Items[Index].Key, Key) == 0) {
      // Replacing the value if already exist
      free(Item.Key);
      free(Table->Items[Index].Value);
      
      Table->Items[Index].Value = Value;
      return true;
    } else {
      // Linear Probing
      Index = (Index+1) % Table->Capacity;      
    }
  }

  Table->Items[Index] = Item;
  Table->Count += 1;
  return true;
}

void* VHashTable_Get(vhash_table* Table, const vhash_key* Key) {
  u64 Hash = VHashTable_Hash(Key);
  u64 Index = Hash % Table->Capacity;
  
  while(Table->Items[Index].Key != NULL) {
    if(strcmp(Table->Items[Index].Key, Key) == 0) {
      return Table->Items[Index].Value;
    } else {
      Index = (Index+1) % Table->Capacity;          
    }
  }

  return NULL;
}

void* VHashTable_Remove(vhash_table* Table, const vhash_key* Key) {
  u64 Hash = VHashTable_Hash(Key);
  u64 Index = Hash % Table->Capacity;

  while(Table->Items[Index].Key != NULL) {
    if(strcmp(Table->Items[Index].Key, Key) == 0) {
      vhash_item* Item = &Table->Items[Index];
      free(Item->Key);
      Item->Key = NULL;
      Table->Count -= 1;
      return Item->Value;
    } else {
      Index = (Index+1) % Table->Capacity;          
    }
  }

  return NULL;
}

u64 VHashTable_Count(vhash_table* Table) {
  return Table->Count;
}

vhash_item *VHashTable_Next(vhash_iterator* Iterator) {
  vhash_item *Item;
  if(Iterator->Cursor >= Iterator->Table->Capacity
     || Iterator->Count == Iterator->Table->Count) {
    return NULL;
  } 

  while(VHashTable_ItemIsEmpty(Iterator->Table, Iterator->Cursor)) {
    Iterator->Cursor += 1;
  }

  Item = &Iterator->Table->Items[Iterator->Cursor];
  Iterator->Cursor += 1;
  Iterator->Count += 1;

  return Item;
}
