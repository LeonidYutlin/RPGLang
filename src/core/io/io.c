#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "io/io.h"
#include "ds/tree/node.h"

static TreeNode* nodeReadRecursion(MappedFile* mf, size_t* p,
                                   Error* status, size_t* nodeCount);
static Error nodePrint(FILE* f, TreeNode* node);

#define NULL_STR "NULL"
static const char* NULL_STRING = NULL_STR;
static size_t NULL_STRING_LEN  = sizeof(NULL_STR) - 1;
#undef  NULL_STR_REP

TreeNode* nodeReadC(MappedFile* mf, Error* status, size_t* nodeCount) {
  if (!mf || !mf->data)
    RETURN_WITH_STATUS(BadArgs, NULL);
  Error err = OK;

  size_t p = 0;
  TreeNode* node = nodeReadRecursion(mf, &p, &err, nodeCount);
  if (err)
    RETURN_WITH_STATUS(err, NULL);
  nodeFixParents(node);
  return node;
}

#define DUMP_ERROR_RETURN(commentary)            \
  {                                              \
  logln(ERROR,                                   \
        "Failed to read node at position %lu\n"  \
        "Comment: %s\n"                          \
        "\tLine snippet:\n"                      \
        "\t->%.*s...",                           \
        *p,                                      \
        commentary,                              \
        mf->size - *p > 10 ? 10 : mf->size - *p, \
        mf->data + *p);                          \
  RETURN_WITH_STATUS(Fail, NULL);                \
  }

#define SKIP_WHITESPACE           \
  while (isspace(mf->data[*p]))   \
      (*p)++;                     \

#define CUR (mf->data + *p)

//NOTE: not sure abt sscanf here
static TreeNode* nodeReadRecursion(MappedFile* mf, size_t* p,
                                   Error* status, size_t* nodeCount) {
  assert(mf);
  assert(mf->data);
  assert(*p < mf->size);

  // fprintf(stderr,
  //             "[INFO]: at position %lu\n"             
  //             "\tLine snippet:\n"
  //             "\t->%.10s...\n",
  //             *p,
  //             buf + *p);
  SKIP_WHITESPACE;
  if (*CUR == '(')
    (*p)++;
  else 
    DUMP_ERROR_RETURN("No opening parenthethis");

  SKIP_WHITESPACE;
  NodeUnit data = {.type = UNKNOWN_TYPE};
  if (strncmp(CUR,
              NULL_STRING, NULL_STRING_LEN) == 0) {
    *p += NULL_STRING_LEN;
    SKIP_WHITESPACE;
    if (*CUR == ')')
      (*p)++;
    else
      DUMP_ERROR_RETURN("No closing parenthethis");
    return NULL;
  }

  long readN = 0;
  if (sscanf(CUR,
             ".exc = %lu "
             ".type = %ln",
             &data.exceptionCount, &readN) != 1) {
    DUMP_ERROR_RETURN(".exc or .type scanf failed");
    RETURN_WITH_STATUS(Fail, NULL);
  }
  *p += (size_t)readN;

  size_t len = (size_t)(strchr(CUR, ' ') - CUR);
  int type = getNodeType(CUR, len);
  if (type < 0)
    DUMP_ERROR_RETURN("Unknown node type");
  *p += len;
  data.type = (NodeType)type;
  SKIP_WHITESPACE;
  sscanf(CUR, ".value = %ln", &readN);
  if (!readN)
    DUMP_ERROR_RETURN("Value field error");
  *p += (size_t)readN;
  SKIP_WHITESPACE;
  switch (data.type) {
    case NUM_TYPE:
      if (sscanf(CUR, "%ld%ln", 
                 &data.value.num, &readN) != 1)
        DUMP_ERROR_RETURN("Num node value error");
      *p += (size_t)readN;
      break;
    case IDENT_TYPE:
      len = (size_t)(strchr(CUR, ' ') - CUR);
      data.value.id = (StringView){.data = CUR, .size = len};
      *p += len;
      break;
    case OP_TYPE:
      len = (size_t)(strchr(CUR, ' ') - CUR);
      type = getOpType(CUR, len);
      if (type < 0)
        DUMP_ERROR_RETURN("Unknown op type");
      *p += len;
      data.value.op = (OpType)type;     
      break;
    case CTRL_TYPE:
      len = (size_t)(strchr(CUR, ' ') - CUR);
      type = getCtrlType(CUR, len);
      if (type < 0)
        DUMP_ERROR_RETURN("Unknown ctrl type");
      *p += len;
      data.value.ctrl = (CtrlType)type;     
      break;
    case VAR_TYPE_TYPE:
      len = (size_t)(strchr(CUR, ' ') - CUR);
      type = getVarTypeType(CUR, len);
      if (type < 0)
        DUMP_ERROR_RETURN("Unknown var type type");
      *p += len;
      data.value.varType = (VarType)type;     
      break;
    default:
      DUMP_ERROR_RETURN("unreacheable");
      break;
  }
  SKIP_WHITESPACE;

  TreeNode* left  = nodeReadRecursion(mf, p, status, nodeCount);
  if (*status) {
    nodeDestroy(left);
    return NULL;
  }
  TreeNode* right = nodeReadRecursion(mf, p, status, nodeCount);
  if (*status) {
    nodeDestroy(left);
    nodeDestroy(right);
    return NULL;
  }

  //fprintf(stderr, "Pos is %lu BufSize is %lu\n", *p, bufSize);
  SKIP_WHITESPACE;
  if (mf->data[*p] == ')') {
    (*p)++;
  } else {
    nodeDestroy(left);
    nodeDestroy(right);
    DUMP_ERROR_RETURN("No closing parenthesis");
  }

  TreeNode* node = nodeAlloc(data, .left = left, .right = right, status);
  if (*status) {
    nodeDelete(left);
    nodeDelete(right);
    nodeDestroy(node);
    return NULL;
  }

  if (nodeCount)
    (*nodeCount)++;
  return node; 
}

#undef DUMP_ERROR_RETURN
#undef SKIP_WHITESPACE
#undef CUR

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
      fprintf(f, "%.*s ", 
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
