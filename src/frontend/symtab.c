#include "frontend/symtab.h"
#include "ds/hashtable/entry.h"
#include "ds/tree/type.h"
#include <ctype.h>
#include <stdlib.h>

static StringView mangleName(StringView name, Error* status);
static Error symtabAddStdlib(HashTable* symtab);
static Error checkCallsCallback(TreeNode* node, uint level, void* data);
static Error populateSymtabCallback(TreeNode* node, uint level, void* data);
static Error convertRawIdentifiersCallback(TreeNode* node, uint level, void* data);

Error symtabInit(TranslationUnit* trUnit, size_t bucketCount, 
                 size_t initialListCapacity, hash_f hashFunc) {
  if (!trUnit ||
      !trUnit->ast)
    return BadArgs;

  Error err = OK;
  if ((err = hashTableInit(&trUnit->symtab, bucketCount, 
                           initialListCapacity, sizeof(Symbol),
                           freeSymbol, printSymbol,
                           cmpSymbol, hashFunc)))
    return err;

  symtabAddStdlib(&trUnit->symtab);
  if (nodeTraverse(trUnit->ast, 
                   .postfix = populateSymtabCallback, 
                   .postfixData = &trUnit->symtab)) {
    fprintf(stderr, "Multiple declarations of the same symbol detected, "
                    "no further compilation is done\n");
    return Fail;
  }
  if (nodeTraverse(trUnit->ast, 
                    .postfix = convertRawIdentifiersCallback,
                    .postfixData = &trUnit->symtab)) {
    fprintf(stderr, "Unknown function call detected, no further compilation is done\n");
    return Fail;
  }
  
  return OK;
}

bool symtabCheckCalls(TranslationUnit* trUnit, Error* status) {
  Error err = OK;
  if ((err = hashTableVerify(&trUnit->symtab)))
    RETURN_WITH_STATUS(err, false);
  if (!trUnit ||
      !trUnit->ast)
    RETURN_WITH_STATUS(BadArgs, false);

  return !nodeTraverse(trUnit->ast, 
                      .postfix = checkCallsCallback,
                      .postfixData = &trUnit->symtab);
}

#define STDLIB_FUNC_LIST() \
  X("out", 1)              \
  X("exit", 1)             \
  X("rout", 2)             \
  X("random", 0)

static Error symtabAddStdlib(HashTable* symtab) {
  Error err = OK;
  if ((err = hashTableVerify(symtab)))
    return err;
  
  Symbol sym = (Symbol){.external = true};
#define X(name, argCount)                       \
  sym.argc = argCount;                          \
  sym.mangledName = mangleName(SV(name), &err); \
  if (err)                                      \
    return err;                                 \
  hashTablePut(symtab, SV(name), &sym);

  STDLIB_FUNC_LIST()

#undef X

  return OK;
}

static Error checkCallsCallback(TreeNode* node,
                                _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!OF_CTRL(node, CTRL_FUNC_CALL))
    return OK;

  Error err = OK;
  HashTable* ht = (HashTable*)data;
  TreeNode* funcIdNode = node->left;
  if (!IS_SYMBOL(funcIdNode))
    return OK;
  SymbolIndex symIdx = funcIdNode->data.value.sym;
  Entry* entry = (Entry*)listGetValue(ht->buckets + symIdx.bucketIndex, 
                                      symIdx.listIndex, &err);
  if (err)
    return err;
  Symbol* sym = (Symbol*)listGetValue(&ht->values, entry->value, &err);
  if (err)
    return err;
  uint64_t argc = 0;
  TreeNode* argNode = node->right;
  while (argNode) {
    argc++;
    argNode = argNode->right;
  }
  if (argc != sym->argc) {
    //TODO: better diagnostics when call is invalid
    fprintf(stderr, 
            "[ERROR] function %.*s expects %lu arguments, but %lu were provided\n",
            (int)entry->key.size, entry->key.data, sym->argc, argc);
    return Fail;
  } 
  return OK;
}

static Error populateSymtabCallback(TreeNode* node, 
                                    _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!OF_CTRL(node, CTRL_FUNC_DECL))
    return OK;

  Error err = OK;
  HashTable* ht = (HashTable*)data;
  TreeNode* funcIdNode = node->left->left->right;
  StringView funcName = funcIdNode->data.value.rawId;
  Symbol sym = (Symbol){};
  if (hashTableGet(ht, funcName, &sym, &err)) {
    fprintf(stderr, 
            "[ERROR]: Function %.*s has already been declared before\n",
            (int)funcName.size, funcName.data);
    return Fail;
  }
  if (err)
    return err;

  uint64_t argc = 0;
  TreeNode* paramNode = node->left->right;
  while (paramNode) {
    argc++;
    paramNode = paramNode->right;
  }
  sym = (Symbol){
    .mangledName = mangleName(funcName, &err),
    .argc = argc,
    .external = false,
  };
  if (err)
    return err;

  size_t bucketIndex  = 0;
  ListIndex listIndex = 0;
  hashTablePutExt(ht, funcName, &sym, &bucketIndex, &listIndex);
  nodeChangeChild(funcIdNode->parent, funcIdNode, 
                  SYMBOL_(bucketIndex, listIndex), NULL);
  return OK;
}

static Error convertRawIdentifiersCallback(TreeNode* node, 
                                           _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!OF_CTRL(node, CTRL_FUNC_CALL))
    return OK;

  node = node->left; //TODO: remove this once we are doing all raw idents
  Error err = OK;
  HashTable* ht = (HashTable*)data;
  StringView* name = &node->data.value.rawId;
  Symbol* sym = NULL;
  size_t bucketIndex  = 0;
  ListIndex listIndex = 0;
  if (!hashTableGetExt(ht, *name, &sym, &bucketIndex, &listIndex, &err)) {
    fprintf(stderr, 
            "[ERROR] Function named \"%.*s\" is undeclared\n",
            (int)name->size, name->data);
    return Fail;
  }
  
  nodeChangeChild(node->parent, node, 
                  SYMBOL_(bucketIndex, listIndex), NULL);
  if (err)
    return err;
  return OK;
}

static const char ESCAPE_CHAR = '_';
static const char HEX_ALPHABET[] = "0123456789ABCDEF";
static const size_t HEX_LEN = sizeof(HEX_ALPHABET) - 1;

static StringView mangleName(StringView name, Error* status) {
  if (!name.data)
    RETURN_WITH_STATUS(BadArgs, name);

  size_t mangledLen = 0;
  for (size_t i = 0; i < name.size; i++) {
    if (name.data[i] == ESCAPE_CHAR) {
      mangledLen += 2;
      continue;
    }
    if (i == 0 && !isalpha(name.data[i]))
      mangledLen += 1;
    if (!isalnum(name.data[i])) {
      mangledLen += 3;
      continue;
    }
    mangledLen++;
  }
  
  char* mangledName = (char*)calloc(mangledLen + 1, sizeof(char));
  if (!mangledName)
    RETURN_WITH_STATUS(FailMemoryAllocation, name);

  char* dest = mangledName;
  for (size_t i = 0; i < name.size; i++) {
    unsigned char c = (unsigned char)name.data[i];
    if (c == ESCAPE_CHAR) {
      *dest++ = ESCAPE_CHAR;
      *dest++ = ESCAPE_CHAR;
      continue;
    }
    if (i == 0 && !isalpha(c))
      *dest++ = ESCAPE_CHAR;
    if (!isalnum(c)) {
      *dest++ = ESCAPE_CHAR;
      *dest++ = HEX_ALPHABET[c / HEX_LEN];
      *dest++ = HEX_ALPHABET[c % HEX_LEN];
      continue;
    }
    *dest++ = name.data[i];
  }

  StringView result = (StringView){
    .data = mangledName,
    .size = mangledLen,
  };
  return result;
}
