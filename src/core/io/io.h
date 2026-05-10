#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "ds/hashtable/hashtable.h"
#include "ds/tree/node.h"
#include "symbol/symbol.h"

extern const size_t SYMTAB_BUCKET_SIZE;
extern const size_t SYMTAB_LIST_CAPACITY;
extern const hash_f SYMTAB_HASH_FUNC;

Error translationUnitRead(MappedFile* src, TranslationUnit* trUnit);
Error translationUnitPrint(FILE* sink, TranslationUnit* trUnit);

Error symtabPrint(FILE* sink, HashTable* symtab);
Error symtabRead(MappedFile* src, HashTable* symtab, size_t* stopPoint);

TreeNode* nodeReadC(MappedFile* src, Error* status, size_t* nodeCount);
#define nodeRead(src, status) nodeReadC(src, status, NULL);

Error nodePrintPrefix(FILE* sink, TreeNode* node);

typedef struct {
  FILE* sink;
  char c;
} NodeIOCallbackData;

/// expects NodeIOCallbackData or FILE as data
Error nodePutcCallback(TreeNode* node, uint level, void* data);
/// expects NodeIOCallbackData as data
Error nodePrintCallback(TreeNode* node, uint level, void* data);

typedef struct {
  callback_f first;
  callback_f second;
  NodeIOCallbackData sharedData;
} NodeSequenceCallbackData;

/// expects NodeSequenceCallbackData as data
Error sequence(TreeNode* node, uint level, void* data);

#endif
