#ifndef SYMTAB_H
#define SYMTAB_H

#include "ds/hashtable/hashtable.h"
#include "ds/tree/node.h"
#include "utils/utils.h"

typedef struct {
  StringView mangledName;
} Symbol;

Error symtabInit(HashTable* symtab, size_t bucketCount, 
                 size_t initialListCapacity, hash_f hashFunc, TreeNode* ast);

#endif
