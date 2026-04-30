//этот файл нуждается в полной переработке
#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "ds/tree/root.h"

//TreeNode* nodeReadC(FILE* src, Error* status, size_t* nodeCount);
//#define nodeRead(src, status) nodeReadC(src, status, NULL);

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
