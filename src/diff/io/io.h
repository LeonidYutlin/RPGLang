#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "diff/context.h"
#include "ds/tree/root.h"

//not context because file isnt the tex output file but instead a "source" file
TreeNode* nodeRead(FILE* file, Variables* vars, Error* status, size_t* nodeCount);
TreeRoot* treeRead(FILE* file, Variables* vars, Error* status);

#define COMMON_FIELDS \
  FILE* sink;         \
  char c
//Extension of NodePutcCallbackData that includes vars field
//which is needed for nodePrint
//This struct is a perfect suit for all 3 callbacks below
typedef struct NodePrintCallbackData {
  COMMON_FIELDS;
  Variables* vars; 
} NodePrintCallbackData;

typedef struct NodePutcCallbackData {
  COMMON_FIELDS;
} NodePutcCallbackData;

#undef COMMON_FIELDS

Error nodePutcCallback(TreeNode* node, void* data, uint level);
Error nodePrintCallback(TreeNode* node, void* data, uint level);
Error nodePutcAndPrintCallback(TreeNode* node, void* data, uint level);

Error nodePrint(FILE* f, Variables* vars, TreeNode* node); 

#endif
