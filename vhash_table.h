#ifndef _V_HASH_TABLE_H_
#define _V_HASH_TABLE_H_

#include "vutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct vhash_table vhash_table;

typedef uint8_t vhash_key;
typedef void vhash_value;
typedef struct {
  vhash_key* Key;
  vhash_value* Value;
  u64 Hash;
} vhash_item;

typedef struct {
  u64 Cursor;
  u64 Count;
  vhash_table* Table;
} vhash_iterator;

/**
 * @brief Inserts a key-value pair into the hash table.
 *
 * This function copies both the key and value into the hash table. If the key already exists,
 * the existing value is replaced with the new one. Value is copied using the provided size,
 * and memory is allocated internally (the table assumes ownership).
 *
 * If the table is at capacity, it is rehashed before inserting.
 *
 * @param Table Pointer to the hash table.
 * @param Key Null-terminated string key to insert. It is duplicated internally.
 * @param Value Pointer to the value to insert. It will be copied using memcpy.
 * @param SizeofValue Size (in bytes) of the value pointed to by `Value`.
 *
 * @return true if the insertion or replacement was successful, false otherwise.
 *
 * @note This function performs a shallow memcpy of the value. It is the caller's responsibility
 *       to ensure that the data at `Value` is valid and trivially copyable.
 *
 */
bool VHashTable_Insert(vhash_table* Table, vhash_key* Key, vhash_value* Value, u64 SizeofValue);

/**
 * Get a value from the hash table
 * @param Table the hash table itself
 * @param Key a key identifier
 * @return Returns the value referenced by the key, if the key is invalid, returns NULL
 *
 * @note The return value if modified, modifies the value in the table
 */
void* VHashTable_Get(vhash_table* Table, const vhash_key* Key);

/**
 * @brief Initializes a new hash table with the specified capacity.
 * 
 * @param Capacity Initial capacity of the hash table. If 0, the default capacity will be used.
 * @return vhash_table* Pointer to the initialized hash table or NULL on failure.
 */
vhash_table* VHashTable_Init(u64 Capacity);

/**
 * @brief Retrieves the value associated with a key in the hash table.
 * 
 * @param Table Pointer to the hash table.
 * @param Key Pointer to the key of the item to be retrieved.
 * @return void* Pointer to the value associated with the key or NULL if not found.
 */
void* VHashTable_Get(vhash_table* Table, const vhash_key* Key);

/**
 * @brief Removes an item from the hash table based on the key.
 * 
 * @param Table Pointer to the hash table.
 * @param Key Pointer to the key of the item to be removed.
 * @return void* Pointer to the removed value or NULL if not found.
 *
 * @note User should free the returned pointer.
 */
void* VHashTable_Remove(vhash_table* Table, const vhash_key* Key);

/**
 * @brief Returns the number of items in the hash table.
 * 
 * @param Table Pointer to the hash table.
 * @return u64 The number of items in the hash table.
 */
u64 VHashTable_Count(vhash_table* Table);

/**
 * @brief Initializes an iterator for the hash table.
 * 
 * @param Table Pointer to the hash table.
 * @return vhash_iterator The initialized iterator.
 */
vhash_iterator VHashTable_Iterator(vhash_table* Table);

/**
 * @brief Retrieves the next item from the hash table iterator.
 * 
 * @param Iterator Pointer to the hash table iterator.
 * @return vhash_item* Pointer to the next item or NULL if there are no more items.
 */
vhash_item *VHashTable_Next(vhash_iterator* Iterator);

/**
 * @brief Frees the memory occupied by the hash table.
 * 
 * @param Table Pointer to the hash table to be freed.
 */
void VHashTable_Free(vhash_table *Table);

#endif

