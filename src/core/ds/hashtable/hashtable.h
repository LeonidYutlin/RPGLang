#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "ds/list/list.h"
#include "error/error.h"
#include "utils/utils.h"
#include <stddef.h>
#include <stdint.h>

typedef uint64_t (*hash_f)(StringView strView);

typedef struct {
  List   values;
  List*  buckets;
  size_t bucketCount;
  hash_f hashFunc;
  bool initialized;
} HashTable;

Error hashTableInit(HashTable* table, size_t bucketCount, 
                    size_t initialListCapacity, size_t itemSize, 
                    free_f freeFunc, printf_f printfFunc, 
                    cmp_f cmpFunc, hash_f hashFunc);
HashTable* hashTableAlloc(size_t bucketCount, size_t initialListCapacity,
                          size_t itemSize, free_f freeFunc,
                          printf_f printfFunc, cmp_f cmpFunc,
                          hash_f hashFunc, Error* status);
Error hashTablePut(HashTable* table, StringView key, void* value);
bool hashTableGet(HashTable* table, StringView key, void** result, Error* status);
Error hashTableDelete(HashTable* table, StringView key);
Error hashTableVerify(HashTable* table);
Error hashTableDestroy(HashTable* table, bool isAlloced);

#endif
