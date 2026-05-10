#include "frontend/symtab.h"
#include "ds/tree/type.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static StringView mangleName(StringView name, Error* status);
static Error populateSymtabCallback(TreeNode* node, uint level, void* data);
static Error convertRawIdentifiersCallback(TreeNode* node, uint level, void* data);
static bool cmpSymbol(void* symA, void* symB);
static void printSymbol(FILE* sink, void* sym);
static void freeSymbol(void* sym); 

Error symtabInit(HashTable* symtab, size_t bucketCount, 
                 size_t initialListCapacity, hash_f hashFunc, TreeNode* ast) {
  Error err = OK;
  if ((err = hashTableInit(symtab, bucketCount, 
                           initialListCapacity, sizeof(Symbol),
                           freeSymbol, printSymbol,
                           cmpSymbol, hashFunc)))
    return err;

  nodeTraverse(ast, 
               .postfix = populateSymtabCallback, 
               .postfixData = symtab);
  nodeTraverse(ast, 
               .postfix = convertRawIdentifiersCallback,
               .postfixData = symtab);
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
  Symbol sym = (Symbol){
    .mangledName = mangleName(funcName, &err),
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
  if (!IS_RAW_IDENT(node))
    return OK;

  Error err = OK;
  HashTable* ht = (HashTable*)data;
  StringView* name = &node->data.value.rawId;
  Symbol* sym = NULL;
  size_t bucketIndex  = 0;
  ListIndex listIndex = 0;
  if (hashTableGetExt(ht, *name, &sym, &bucketIndex, &listIndex, &err))
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

static bool cmpSymbol(void* symA, void* symB) {
  if ((!symA &&  symB) ||
      ( symA && !symB))
    return false;
  if (!symA && !symB)
    return true;

  Symbol* a = symA;
  Symbol* b = symB;
  return a->mangledName.size == b->mangledName.size &&
         strncmp(a->mangledName.data, b->mangledName.data, a->mangledName.size);
}

static void printSymbol(FILE* sink, void* sym) {
  if (!sink || !sym)
    return;

  Symbol* s = sym;
  fprintf(sink, 
          "mangledName = %.*s\n",
          (int)s->mangledName.size, s->mangledName.data);
}

static void freeSymbol(void* sym) {
  if (!sym)
    return;

  Symbol* s = sym;
  free(s->mangledName.data);
}
