#ifndef SYMTAB_H
#define SYMTAB_H

#include "ds/hashtable/hashtable.h"
#include "ds/tree/node.h"
#include "utils/utils.h"

typedef struct {
  HashTable symtab;
  TreeNode* ast;
} TranslationUnit;

typedef struct {
  StringView mangledName;
  uint64_t argc;
  bool external;
} Symbol;

Error symtabInit(TranslationUnit* trUnit, size_t bucketCount, 
                 size_t initialListCapacity, hash_f hashFunc);

bool symtabCheckCalls(TranslationUnit* trUnit, Error* status);

#endif
