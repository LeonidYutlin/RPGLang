//этот файл нуждается в полной переработке

#include <stdlib.h>
#include <string.h>
#include "diff/context.h"
#include "diff/io/io.h"

static Variable* findVarHashed(Variables* vars, const char* varStr, 
                               ulong hash, Error* status, 
                               size_t* indexPtr);
static ulong hash(const char* str);

Error contextInit(Context* ctx, size_t initialCapacity) {
  if (!ctx ||
      initialCapacity == 0)
    return InvalidParameters;

  Error err = OK;
  //if varsAlloc fails it already freed the memory
  Variables* vars = varsAlloc(initialCapacity, &err);
  if (err)
    return err;

  ctx->vars = vars;
  ctx->sink = NULL;
  ctx->stepCount = 0;

  return OK;
}

Error contextDestroy(Context* ctx) {
  if (!ctx)
    return InvalidParameters;
  
  if (ctx->vars)
    varsDestroy(ctx->vars);
  //if (ctx->sink)
    //closeTexFile(ctx);
  ctx->stepCount = 0;
  return OK;
}

Error contextVerify(Context* ctx) {
  if (!ctx)
    return InvalidParameters;
  if (!ctx->vars || !ctx->sink)
    return NullPointerField;
  return varsVerify(ctx->vars);
}

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
      *status = value;                         \
  return returnValue;                          \
  }

Variables* varsAlloc(size_t initialCapacity, Error* status) {
  if (!initialCapacity)
    RETURN_WITH_STATUS(InvalidParameters, NULL);

  Variables* vars = (Variables*)calloc(1, sizeof(Variables));
  if (!vars)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Variable* items = (Variable*)calloc(initialCapacity, sizeof(Variable));
  if (!items) {
    free(vars);
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);
  }

  vars->items = items;
  vars->count = 0;
  vars->capacity = initialCapacity;

  return vars;
}

Error varsDestroy(Variables* vars) {
  if (!vars)
    return InvalidParameters;

  if (vars->items) {
    for (size_t i = 0; i < vars->capacity; i++)
      free(vars->items[i].str);
  }
  free(vars->items);
  free(vars);

  return OK;
}

size_t regVar(Variables* vars, const char* varStr, Error* status) {
  if (!vars || !varStr)
    RETURN_WITH_STATUS(InvalidParameters, 0);
  Error err = varsVerify(vars);
  if (err)
    RETURN_WITH_STATUS(err, 0);

  ulong strhash = hash(varStr);
  //does a variable with that str exist already
  if (findVarHashed(vars, varStr, strhash, NULL, NULL))
    RETURN_WITH_STATUS(AttemptedReregistration, 0);
 
  if (vars->count == vars->capacity) {
    size_t newCapacity = vars->capacity * 2;
    Variable* temp = (Variable*)realloc(vars->items, newCapacity * sizeof(Variable));
    if (!temp)
      RETURN_WITH_STATUS(FailMemoryReallocation, 0);
    vars->capacity = newCapacity;
    vars->items = temp;
  }

  char* str = strdup(varStr);
  if (!str)
    RETURN_WITH_STATUS(FailMemoryAllocation, 0);

  size_t retVal = vars->count;
  vars->items[vars->count++] = (Variable){
    .hash = strhash,
    .str = str, 
    .value = NAN
  };
  return retVal;
}

Variable* getVar(Variables* vars, size_t index, Error* status) {
  if (!vars || index >= vars->capacity)
    RETURN_WITH_STATUS(InvalidParameters, NULL);
  Error err = varsVerify(vars);
  if (err)
    RETURN_WITH_STATUS(err, NULL);

  return vars->items + index;
}

Error setVarValue(Variables* vars, const char* varStr, double value) {
  if (!vars || !varStr)
    return InvalidParameters;
  Error err = varsVerify(vars);
  if (err)
    return err;

  Variable* v = findVar(vars, varStr, NULL, NULL);
  if (v) {
    v->value = value;
  } else {
    return UnknownVariable; 
  }

  return OK;
}

Variable* findVar(Variables* vars, const char* varStr, 
                  Error* status, size_t* indexPtr) {
  if (!vars || !varStr)
    RETURN_WITH_STATUS(InvalidParameters, NULL);
  Error err = varsVerify(vars);
  if (err)
    RETURN_WITH_STATUS(err, NULL);

  return findVarHashed(vars, varStr, hash(varStr), status, indexPtr);
}

bool ofVar(Variables* vars, TreeNode* node, const char* varStr) {
  if (!varStr ||
      !node   ||
      !vars)
    return false;

  Variable* v = getVar(vars, (node)->data.value.var, NULL);
  if (!v)
    return false;

  return (IS_VAR((node)) && 
          strcmp(v->str, varStr) == 0);
}

Error varsVerify(Variables* vars) {
  if (!vars)
    return InvalidParameters;
  if (!vars->items)
    return NullPointerField;
  if (vars->count > vars->capacity)
    return BadCount;
  return OK;
}

static Variable* findVarHashed(Variables* vars, const char* varStr, 
                               ulong hash, Error* status, size_t* indexPtr) {
  if (!vars || !varStr)
    RETURN_WITH_STATUS(InvalidParameters, NULL);
  Error err = varsVerify(vars);
  if (err)
    RETURN_WITH_STATUS(err, NULL);

  for (size_t i = 0; i < vars->count; i++) {
    Variable* vi = vars->items + i;
    if (!vi->str) continue;
    if (hash == vi->hash && 
        strcmp(varStr, vi->str) == 0) {
      if (indexPtr) 
        *indexPtr = i;
      return vi;
    }
  }

  RETURN_WITH_STATUS(UnknownVariable, NULL);
}

//NOTE: Source: http://www.cse.yorku.ca/~oz/hash.html
static ulong hash(const char* str) {
  // const char* strSaved = str;
  ulong hash = 5381;
  uint c = 0;

  while ((c = (uint)*str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  // fprintf(stderr, "%s is %lu\n", strSaved, hash);
  return hash;
}

#undef RETURN_WITH_STATUS
