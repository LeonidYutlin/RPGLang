#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include "ds/tree/node/node.h"

//this isn't static const but a macro because of how C treats
//constant-variable sized arrays (for some reason mistakes them for VLAs)
#define MAX_VALUE_STRING_LENGTH 16

typedef struct Variable {
  ulong hash;
  char* str;
  double value;
} Variable;

typedef struct Variables {
  Variable* items;
  size_t capacity;
  size_t count;
} Variables;

Variables* varsAlloc(size_t initialCapacity, Error* status);
Error varsDestroy(struct Variables* vars); 
size_t regVar(Variables* vars, const char* varStr, Error* status); 
Variable* getVar(Variables* vars, size_t index, Error* status);
Variable* findVar(Variables* vars, const char* varStr, 
                  Error* status, size_t* indexPtr);
//NOTE: any error (e.g. NULL vars) and this will return false
bool ofVar(Variables* vars, TreeNode* node, const char* varStr);
Error setVarValue(Variables* vars, const char* varStr, double value);
Error varsVerify(Variables* vars);

typedef struct Context {
  FILE* sink;
  Variables* vars;
  uint stepCount;
} Context;

Error contextInit(Context* context, size_t initialCapacity);
Error contextDestroy(Context* context);

Error contextVerify(Context* context);

#define OF_VAR(vars, node, i) ofVar(vars, node, i)

#endif
