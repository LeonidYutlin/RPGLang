//этот файл нуждается в полной переработке

#include <assert.h>
#include "io/io.h"
#include "ds/tree/node.h"

//static TreeNode* nodeReadRecursion(char* buf, size_t bufSize, size_t* p,
//                                   Error* status, size_t* nodeCount);

static Error nodePrint(FILE* f, TreeNode* node);


#define NULL_STR "NULL"
static const char* NULL_STRING = NULL_STR;
static size_t NULL_STRING_LEN  = sizeof(NULL_STR) - 1;
#undef  NULL_STR_REP

/*
TreeNode* nodeReadC(FILE* f, Error* status, size_t* nodeCount) {
  if (!f)
    RETURN_WITH_STATUS(BadArgs, NULL);
  Error err = OK;

  MappedFile mf = {};
  if ((err = mappedFileInit(fileno(f), &mf)))
    RETURN_WITH_STATUS(err, NULL);

  size_t p = 0;
  TreeNode* node = nodeReadRecursion(vars, mf.data, mf.size, &p, &err, nodeCount);
  mappedFileDestroy(&mf);
  if (err)
    RETURN_WITH_STATUS(err, NULL);
  return node;
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
    RETURN_WITH_STATUS(BadArgs, NULL);
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
*/

Error nodePrintPrefix(FILE* sink, TreeNode* node) {
  if (!sink || !node)
    return BadArgs;

  NodeSequenceCallbackData prefixData = {
    .first  = nodePutcCallback,
    .second = nodePrintCallback,
    .sharedData = {
      .sink = sink, 
      .c = '('
    },
  };
  NodeIOCallbackData infixData = {
    .sink = sink, 
  };
  NodeIOCallbackData postfixData = {
    .sink = sink, 
    .c = ')'
  };
  nodeTraverse(node,
               .prefix  = sequence,
               .infix   = NULL,
               .postfix = nodePutcCallback,
               .prefixData  = &prefixData,
               .infixData   = &infixData,
               .postfixData = &postfixData);

  return OK;
}

Error nodePutcCallback(_unused TreeNode* node,
                       _unused uint level, void* data) {
  NodeIOCallbackData* d = (NodeIOCallbackData*)data;
  if (!d || !d->sink)
    return BadArgs;

  return (fputc(d->c, d->sink) == EOF)
         ? FileError
         : OK;
}

Error nodePrintCallback(TreeNode* node, 
                        _unused uint level, void* data) {
  NodeIOCallbackData* d = (NodeIOCallbackData*)data;
  if (!d || !d->sink)
    return BadArgs;

  return nodePrint(d->sink, node);
}

Error sequence(TreeNode* node, 
               _unused uint level, void* data) {
  NodeSequenceCallbackData* d = (NodeSequenceCallbackData*)data;
  if (!d || 
      !d->first || 
      !d->second)
    return BadArgs;

  return d->first(node, level, &d->sharedData) ||
         d->second(node, level, &d->sharedData);
}

static Error nodePrint(FILE* f, TreeNode* node) {
  if (!f)
    return BadArgs;

  if (!node) {
    fprintf(f, "%s", NULL_STRING);
    return OK;
  } else {
    fprintf(f, 
            ".exc = %lu "
            ".type = %s "
            ".value = ", 
            node->data.exceptionCount,
            getNodeTypeStr(node->data.type));
  }
  switch (node->data.type) {
    case NUM_TYPE:
      fprintf(f, "%ld ", node->data.value.num); 
      break;
    case IDENT_TYPE:
      fprintf(f, "\"%.*s\" ", 
              (int)node->data.value.id.size, 
              node->data.value.id.data); 
      break;
    case OP_TYPE:
      {
        const OpTypeInfo* i = parseOpType(node->data.value.op);
        assert(i);
        fprintf(f, "%s ", i->str); 
      }
      break;
    case CTRL_TYPE:
      {
        const char* i = getCtrlTypeStr(node->data.value.ctrl);
        assert(i);
        fprintf(f, "%s ", i); 
      }
      break;
    case VAR_TYPE_TYPE:
      {
        const char* i = getVarTypeStr(node->data.value.varType);
        assert(i);
        fprintf(f, "%s ", i); 
      }
      break;
    default: break;
  }
  return OK;
}
