#include "ds/tree/tree.h"
#include "diff/io/io.h"
#include "misc/utils.h"
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static bool compareParentPriority(TreeNode* node);
static TreeNode* nodeReadRecursion(Variables* vars, 
                                   char* buf, size_t bufSize, size_t* p,
                                   Error* status, size_t* nodeCount);

#define NULL_STR_REP "nil"
static const char* NULL_STRING_REPRESENTATION   = NULL_STR_REP;
static size_t NULL_STRING_REPRESENTATION_LENGTH = sizeof(NULL_STR_REP) - 1;
#undef  NULL_STR_REP

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
    *status = value;                           \
  return returnValue;                          \
  }

TreeNode* nodeRead(FILE* f, Variables* vars, Error* status, size_t* nodeCount) {
  if (!f ||
      !vars)
    RETURN_WITH_STATUS(InvalidParameters, NULL);
  Error err = OK;
  if ((err = varsVerify(vars)))
    RETURN_WITH_STATUS(err, NULL);

  char* buffer = NULL;
  size_t bufferSize = 0;
  if ((err = readBufferFromFile(f, &buffer, &bufferSize)))
    RETURN_WITH_STATUS(err, NULL);

  size_t p = 0;
  TreeNode* node = nodeReadRecursion(vars, buffer, bufferSize, &p, &err, nodeCount);
  free(buffer);
  if (err)
    RETURN_WITH_STATUS(err, NULL);
  return node;
}

TreeRoot* treeRead(FILE* f, Variables* vars, Error* status) {
  if (!f ||
      !vars)
    RETURN_WITH_STATUS(InvalidParameters, NULL);
  Error err = OK;
  if ((err = varsVerify(vars)))
    RETURN_WITH_STATUS(err, NULL);

  size_t nodeCount = 0;
  TreeNode* node = nodeRead(f, vars, &err, &nodeCount);
  if (err) {
    nodeDestroy(node);
    RETURN_WITH_STATUS(err, NULL);
  }

  TreeRoot* root = attachRootC(node, nodeCount, &err);
  if (err)
    RETURN_WITH_STATUS(err, NULL);

  return root;
}

#define DUMP_ERROR_RETURN(commentary)                      \
  {                                                        \
  fprintf(stderr,                                          \
          "[ERROR]: Failed to read node at pition %lu\n" \
          "Comment: %s\n"                                  \
          "\tLine snippet:\n"                              \
          "\t->%.10s...\n",                                \
          *p,                                            \
          commentary,                                      \
          buf + *p);                                     \
  RETURN_WITH_STATUS(FailReadNode, NULL);                  \
  }

#define SKIP_WHITESPACE      \
  while (isspace(buf[*p])) { \
      (*p)++;                \
  }

static TreeNode* nodeReadRecursion(Variables* vars,
                                   char* buf, size_t bufSize, size_t* p,
                                   Error* status, size_t* nodeCount) {
  if (!buf || 
      *p >= bufSize || 
      !vars)
    RETURN_WITH_STATUS(InvalidParameters, NULL);
  Error err = OK;
  if ((err = varsVerify(vars)))
    RETURN_WITH_STATUS(err, NULL);

  // fprintf(stderr,
  //             "[INFO]: at position %lu\n"             
  //             "\tLine snippet:\n"
  //             "\t->%.10s...\n",
  //             *p,
  //             buf + *p);
  SKIP_WHITESPACE;
  if (buf[*p] == '(') {
    (*p)++;
    SKIP_WHITESPACE;
    NodeUnit data = {};
    int charReadN = 0;
    if (isdigit(buf[*p]) ||
        (buf[*p] == '-' && isdigit(buf[*p + 1]))) {
      if (sscanf(buf + *p,
                 "%lg%n",
                 &data.value.num, &charReadN) != 1)
        DUMP_ERROR_RETURN("No valid value in node");
      data.type = NUM_TYPE;
      *p += (size_t)charReadN;
    } else {
      char valStr[MAX_VALUE_STRING_LENGTH] = {0};
      //char* valBuf = (char*)calloc(BASE_VALUE_BUF_LEN, sizeof(char));
      //if (!valBuf)
      //  DUMP_ERROR_RETURN("Memory allocation failed");
      //while (...)
      //
      //OVERFLOW, bad sscanf - use smth else 
      if (sscanf(buf + *p, "%s%n", 
                 valStr, &charReadN) != 1)
        DUMP_ERROR_RETURN("No valid var/op value in node");
      if (valStr[MAX_VALUE_STRING_LENGTH - 1] != '\0')
        DUMP_ERROR_RETURN("Name of a variable or operator is too long")
      int opType = getOpType(valStr);
      //fprintf(stderr, "returned opType %d\n", opType);
      if (opType >= 0) {
        data.value.op = (OpType)opType;
        data.type = OP_TYPE;
        *p += (size_t)charReadN;
      } else {
        size_t index = regVar(vars, valStr, &err);
        if (err != OK &&
            err != AttemptedReregistration)
          RETURN_WITH_STATUS(err, NULL);
        data.value.var = index;
        data.type = VAR_TYPE;
        *p += (size_t)charReadN;
      }
    }
    if (data.type == UNKNOWN_TYPE)
      DUMP_ERROR_RETURN("No value in node");

    uint expectedChildN = data.type == OP_TYPE
                          ? parseOpType(data.value.op)->argCount
                          : 0;

    TreeNode* left  = nodeReadRecursion(vars, buf, bufSize, p, status, nodeCount);
    if (*status) {
      nodeDestroy(left);
      return NULL;
    }
    TreeNode* right = nodeReadRecursion(vars, buf, bufSize, p, status, nodeCount);
    if (*status) {
      //Я знаю что тут всегда ноды и так нулевой указатель
      //однако на будущие случаи лучше иметь деструктор чем не иметь
      nodeDestroy(left);
      nodeDestroy(right);
      return NULL;
    }

    uint childN = (uint)(left != NULL) + (uint)(right != NULL);
    if (expectedChildN != childN) {
      nodeDestroy(left); 
      nodeDestroy(right);
      DUMP_ERROR_RETURN("Node has too few or too many null children!");
    }

    //fprintf(stderr, "Pos is %lu BufSize is %lu\n", *p, bufSize);
    SKIP_WHITESPACE;
    if (buf[*p] == ')') {
      (*p)++;
    } else {
      nodeDestroy(left);
      nodeDestroy(right);
      DUMP_ERROR_RETURN("No closing parenthesis");
    }

    TreeNode* node = nodeAlloc(data, NULL, left, right, status);
    if (*status) {
      nodeDelete(left);
      nodeDelete(right);
      nodeDestroy(node);
      return NULL;
    }

    if (node->left)
      node->left->parent  = node;
    if (node->right)
      node->right->parent = node;

    if (nodeCount)
      (*nodeCount)++;
    return node;
  } else if (strncmp(buf + *p,
                     NULL_STRING_REPRESENTATION,
                     NULL_STRING_REPRESENTATION_LENGTH) == 0) {
    *p += NULL_STRING_REPRESENTATION_LENGTH;
    return NULL;
  }

  DUMP_ERROR_RETURN("Illegal character at the start of a node declaration");
}

#undef DUMP_ERROR_RETURN
#undef SKIP_WHITESPACE
#undef RETURN_WITH_STATUS

Error nodePutcCallback(unused TreeNode* node, 
                       void* data,
                       unused uint level) {
  if (!data)
    return InvalidParameters;

  NodePutcCallbackData* d = (NodePutcCallbackData*)data;
  if (!d->sink)
    return InvalidParameters;
  return (fputc(d->c, d->sink) == EOF)
         ? EndOfFile
         : OK;
}

Error nodePrintCallback(TreeNode* node, void* data, 
                        unused uint level) {
  if (!data)
    return InvalidParameters;

  NodePrintCallbackData* d = (NodePrintCallbackData*)data;
  return nodePrint(d->sink, d->vars, node);
}

Error nodePutcAndPrintCallback(TreeNode* node, void* data, 
                              unused uint level) {
  return nodePutcCallback(node, data, level) ||
         nodePrintCallback(node, data, level);
}

Error nodePrint(FILE* f, Variables* vars, TreeNode* node) {
  if (!node ||
      !f ||
      !vars)
    return InvalidParameters;
  Error err = OK;
  if ((err = varsVerify(vars)))
    return err;

  switch (node->data.type) {
    case NUM_TYPE: fprintf(f, "%lf", node->data.value.num); break;
    case VAR_TYPE: 
    {
      Variable* v = getVar(vars, node->data.value.var, NULL);
      fprintf(f, "%s", v ? v->str : "ERROR: Invalid index inside VAR node"); 
    }
    break;
    case OP_TYPE : 
    {
      const OpTypeInfo* i = parseOpType(node->data.value.op);
      fprintf(f, "%s", i ? i->str : "ERROR: Unknown Op Type"); 
    }
    break;
    default: break;
  }
  return OK;
}

///Returns true if parent has higher priority than the node
static bool compareParentPriority(TreeNode* node) {
  assert(node);
  assert(IS_OP(node) &&
         IS_OP(node->parent));

  const OpTypeInfo* par  = parseOpType(node->parent->data.value.op);
  const OpTypeInfo* self = parseOpType(node->data.value.op);
  assert(par && self);
  return par->priority > self->priority;
}
