#include "ds/hashtable/hashtable.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static ListIndex listFindByKey(List* lst, StringView key);

Error hashTableInit(HashTable* table, size_t bucketCount, 
                    size_t initialListCapacity, hash_f hashFunc) {
  Error err = hashTableVerify(table);
  if (err != OK &&
      err != Uninitialized)
    return err;
  if (table->initialized)
    return DenyReinit;
  if (!bucketCount || 
      !initialListCapacity ||
      !hashFunc)
    return BadArgs;

  List* buckets = (List*)calloc(bucketCount, sizeof(List));
  if (!buckets)
    return FailMemoryAllocation;

  for (size_t i = 0; i < bucketCount; i++) {
    if ((err = listInit(buckets + i, initialListCapacity, true))) {
        free(buckets);
        return err;
    }
  }

  *table = (HashTable){
    .buckets = buckets,
    .bucketCount = bucketCount,
    .hashFunc = hashFunc,
    .initialized = true,
  };

  return OK;
}

HashTable* hashTableAlloc(size_t bucketCount, size_t initialListCapacity, 
                          hash_f hashFunc, Error* status) {
  if (!bucketCount ||
      !initialListCapacity ||
      !hashFunc)
    RETURN_WITH_STATUS(BadArgs, NULL);

  HashTable* table = (HashTable*)calloc(1, sizeof(HashTable));
  if (!table)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = hashTableInit(table, bucketCount, 
                           initialListCapacity, hashFunc))) {
    free(table);
    RETURN_WITH_STATUS(err, NULL);
  }

  return table;
}

Error hashTablePut(HashTable* table, StringView key, TokenType value) {
  Error err = OK;
  if ((err = hashTableVerify(table)))
    return err;
  if (!key.data || !key.size)
    return BadArgs;
  
  uint64_t hash = table->hashFunc(key); 
  List* bucket = table->buckets + (hash % table->bucketCount);
  ListIndex index = listFindByKey(bucket, key);
  if (!index) {
    Entry entry = (Entry) {
      .key = key,
      .value = value,
      .hash = hash,
    };
    listAddAfterTail(bucket, entry, &err);
    if (err)
      return err;
  } else {
    (bucket->data + index)->value = value;
  }

  return OK;
}

TokenType hashTableGet(HashTable* table, StringView key, Error* status) {
  Error err = OK;
  if ((err = hashTableVerify(table)))
    RETURN_WITH_STATUS(err, 0);
  if (!key.data || !key.size)
    RETURN_WITH_STATUS(BadArgs, 0);
  
  uint64_t hash = table->hashFunc(key);
  List* bucket = table->buckets + (hash % table->bucketCount);
  ListIndex index = listFindByKey(bucket, key);
  if (!index)
    RETURN_WITH_STATUS(NotFound, 0);
  return (bucket->data + index)->value;
}

Error hashTableDelete(HashTable* table, StringView key) {
  Error err = OK;
  if ((err = hashTableVerify(table)))
    return err;
  if (!key.data || !key.size)
    return BadArgs;
  
  uint64_t hash = table->hashFunc(key);
  List* bucket = table->buckets + (hash % table->bucketCount);
  ListIndex index = listFindByKey(bucket, key);
  if (index)
    listDelete(bucket, index);

  return OK;
}

Error hashTableVerify(HashTable* table) {
  if (!table)
    return BadArgs;
  if (!table->initialized)
    return Uninitialized;
  if (!table->buckets ||
      !table->hashFunc) 
    return NullPointerField;
  if (!table->bucketCount)
    return ZeroSize;
  Error err = OK;
  for (size_t i = 0; i < table->bucketCount; i++) {
    if ((err = listVerify(table->buckets + i)))
      return err;
  }
  return OK;
}

Error hashTableDestroy(HashTable* table, bool isAlloced) {
  if (!table)
    return BadArgs;

  if (table->buckets) {
    for (size_t i = 0; i < table->bucketCount; i++) {
      listDestroy(table->buckets + i, false);
    }
  }
  free(table->buckets);

  *table = (HashTable){};

  if (isAlloced)
    free(table);
  return OK;
}

static ListIndex listFindByKey(List* lst, StringView key) {
  assert(lst);
  assert(key.data);
  assert(key.size);

  for (ListIndex cur = lst->next[0];
       lst->next[cur] < lst->capacity;
       cur = lst->next[cur]) {
    if (!cur)
      break;
    ListUnit* unit = lst->data + cur;
    //logln(INFO, "Comparing %.*s and %.*s", key.size, key.data, unit->key.size, unit->key.data);
    if (unit->key.size == key.size &&
        strncmp(unit->key.data, key.data, key.size) == 0) {
      //logln(INFO, "Success");
      return cur;
    }
  }

  //logln(INFO, "Not Found");
  return 0;
}
