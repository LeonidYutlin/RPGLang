#include "ds/tree/tree.h"
#include "diff/io/io.h"
#include "misc/util.h"
#include "misc/quotes.h"
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static Error nodeToTexTraverse(Context* ctx, TreeNode* node,
                               size_t* writtenCount,
                               bool suppressBrackets, 
                               bool suppressNewline);
static bool compareParentPriority(TreeNode* node);

static TreeNode* nodeReadRecursion(Variables* vars, 
                                   char* buf, size_t bufSize, size_t* p,
                                   Error* status, size_t* nodeCount);

#define NULL_STR_REP "nil"
static const char* NULL_STRING_REPRESENTATION   = NULL_STR_REP;
static size_t NULL_STRING_REPRESENTATION_LENGTH = sizeof(NULL_STR_REP) - 1;
#undef  NULL_STR_REP

static const size_t MAX_CHAR_PER_LINE = 54;

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
    *status = value;                           \
  return returnValue;                          \
  }

Error nodeToTex(Context* ctx, TreeNode* node) {
  Error err = OK;
  if ((err = contextVerify(ctx)))
    return err;

  ctx->stepCount = 1;

  fprintf(ctx->sink,
          "\\raggedright(%u):\\begin{align*}\n",
          ctx->stepCount);
  size_t writtenCount = 0;
  nodeToTexTraverse(ctx, node, &writtenCount, false, false);
  fputs("\n\\end{align*}\\\\\n", ctx->sink);

  return OK;
}

TreeNode* differentiationStepToTex(Context* ctx, const char* var, 
                                   TreeNode* before, TreeNode* after,
                                   Error* status) {
  if (!var || 
      !before || 
      !after ||
      !ctx)
    RETURN_WITH_STATUS(InvalidParameters, NULL);
  Error err = OK;
  if ((err = contextVerify(ctx)))
    RETURN_WITH_STATUS(err, NULL);

  nodeFixParents(after);
  ctx->stepCount++;
  fprintf(ctx->sink,
          "\\raggedright(%u):\\begin{align*}\n\\frac{d}{d%s}(",
          ctx->stepCount, var);
  size_t writtenCount = 0;
  nodeToTexTraverse(ctx, before, &writtenCount, true, false);
  fputs(") = ", ctx->sink);
  // nodeToTexTraverse(after, ctx->sink, &writtenCount);
  // fputs(" = ", ctx->sink);
  nodeOptimize(&after);
  nodeToTexTraverse(ctx, after, &writtenCount, false, false);
  fputs("\n\\end{align*}\\\\\n", ctx->sink);
  return after;
}

Error treeToTex(Context* ctx, TreeRoot* root) {
  if (!root ||
      !ctx)
    return InvalidParameters;
  Error err = OK;
  if ((err = contextVerify(ctx)))
    return err;

  return nodeToTex(ctx, root->rootNode);
}

Error openTexFile(Context* ctx) {
  if (!ctx)
    return InvalidParameters;

  time_t timeAbs = time(NULL);
  char* name = getTimestampedString(".log/", ".tex", 0);
  if (!name)
    return FailMemoryAllocation;

  ctx->sink = fopen(name, "w");
  if (!ctx->sink) {
    free(name);
    return FailFileOpen;
  }
  srand((uint)timeAbs);

  fprintf(ctx->sink,
          "\\documentclass{article}"
          "\\usepackage{amsmath}"
          "\\usepackage{geometry}"
          "\\usepackage{graphicx}"
          "\\usepackage[colorlinks=true, linkcolor=blue, urlcolor=blue]{hyperref}"
          "\\usepackage{enumitem}"
          "\\geometry{a4paper, margin=1in}"
          "\\DeclareMathOperator{\\arccot}{arccot}"
          "\\begin{document}"
          "\\section{Differentiating stuff}"
          "%s\\\\"
          "{Quote: %s\\\\}",
          name + strlen(".log/"),
          QUOTES[(unsigned long)random()
                      % (sizeof(QUOTES) / sizeof(char *))]);
  free(name);
  return OK;
}

Error closeTexFile(Context* ctx) {
  if (!ctx)
    return InvalidParameters;
  if (!ctx->sink)
    return NullPointerField;

  fputs("\\end{document}",
        ctx->sink);
  fclose(ctx->sink);
  ctx->sink = NULL;
  return OK;
}

#ifndef DISABLE_NEWLINES
#define ADD_TO_COUNT(x)           \
  {                               \
  if (writtenCount)               \
      *writtenCount += (size_t)x; \
  }
#else
#define ADD_TO_COUNT(x)
#endif

static Error nodeToTexTraverse(Context* ctx, TreeNode* node, size_t* writtenCount,
                              bool suppressBrackets, bool suppressNewline) {
	if (!node ||
      !ctx)
    return InvalidParameters;
  Error err = OK;
  if ((err = contextVerify(ctx)))
    return err;

  //node needs brackets if it's parent exists, we don't suppress brackets,
  //and the node is either a negative number, it's parent is a supported function
  //(the parent being OP is implied by it being a parent)
  //or it is an operator of a lower priority than it's parent
  bool needsBrackets = (node->parent &&
                        !suppressBrackets &&
                        ((IS_NUM(node) && node->data.value.num < 0) ||
                          parseOpType(node->parent->data.value.op)->isSupported ||
                          (IS_OP(node) && compareParentPriority(node))));
  bool isDivision = OF_OP(node, OP_DIV);
  bool isLog      = (!isDivision &&
                     OF_OP(node, OP_LOG));
  // is this an expression of type (smth)^(1/number)
  bool isRoot     = (!isDivision &&
                     !isLog &&
                     OF_OP(node, OP_POW) &&
                     OF_OP(node->right, OP_DIV) &&
                     OF_NUM(node->right->left, 1) &&
                     IS_NUM(node->right->left));

  if (needsBrackets) {
    fputc('(', ctx->sink);
    ADD_TO_COUNT(1);
  }

  if (isDivision) {
    fputs("\\frac{", ctx->sink);
    nodeToTexTraverse(ctx, node->left, writtenCount, true, true);
    fputs("}{", ctx->sink);
    nodeToTexTraverse(ctx, node->right, writtenCount, true, true);
    fputc('}', ctx->sink);
  } else if (isLog) {
    fputs("\\log_{", ctx->sink);
    nodeToTexTraverse(ctx, node->left, writtenCount, true, true);
    fputs("}", ctx->sink);
    nodeToTexTraverse(ctx, node->right, writtenCount, false, suppressNewline);
  } else if (isRoot) {
    if (doubleEqual(node->right->right->data.value.num, 2)) {
      fputs("\\sqrt{", ctx->sink);
    } else {
      fputs("\\sqrt[", ctx->sink);
      nodeToTexTraverse(ctx, node->right->right, writtenCount, true, true);
      fputs("]{", ctx->sink);
    }
    nodeToTexTraverse(ctx, node->left, writtenCount, true, true);
    fputc('}', ctx->sink);
  } else {
    bool isPow = OF_OP(node, OP_POW);
    if (isPow) fputc('{', ctx->sink);
	  nodeToTexTraverse(ctx, node->left, writtenCount, false, suppressNewline);
    if (isPow) fputc('}', ctx->sink);
    switch (node->data.type) {
      case NUM_TYPE: {
        long written = 0;
        fprintf(ctx->sink, "%lg%ln", node->data.value.num, &written);
        ADD_TO_COUNT(written);
      }
      break;
      case VAR_TYPE: {
        Variable* v = getVar(ctx->vars, node->data.value.var, &err);
        if (err) {
          fprintf(stderr, "%s: %s\n", parseError(err)->str, parseError(err)->desc);
          fputs("Error: invalid var index", ctx->sink);
          return err;
        }
        fprintf(ctx->sink, "%s", v->str);
        ADD_TO_COUNT(1);
      }
      break;
      case OP_TYPE: {
        OpType opType = node->data.value.op;
        long written = 0;
        const OpTypeInfo* i = parseOpType(opType);
        if (!i) {
          fputs("Error: unknown op type", ctx->sink);
          return UnknownEnumItem; 
        }
        fprintf(ctx->sink, "%s%s%ln",
                    i->isSupported ? "\\" : "", i->str, &written);
        ADD_TO_COUNT(written);
      }
      break;
      default:
        fputs("Error: unknown node type", ctx->sink);
        return BadEnumItem;
    }
    if (isPow) fputc('{', ctx->sink);
    nodeToTexTraverse(ctx, node->right, writtenCount, isPow, isPow || suppressNewline);
    if (isPow) fputc('}', ctx->sink);
  }

  if (needsBrackets) {
    fputc(')', ctx->sink);
    ADD_TO_COUNT(1);
  }

  #ifndef DISABLE_NEWLINES
  if (!suppressNewline &&
      writtenCount &&
      *writtenCount > MAX_CHAR_PER_LINE) {
    fputs("\\\\\n", ctx->sink);
    *writtenCount = 0;
  }
  #endif

  return OK;
}

#undef ADD_TO_COUNT

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

  TreeRoot* root = treeAlloc((NodeUnit){}, NULL, NULL, &err);
  if (err)
    RETURN_WITH_STATUS(err, NULL);

  root->nodeCount = 0;
  TreeNode* node = nodeRead(f, vars, &err, &root->nodeCount);
  if (err) {
    treeDestroy(root, true);
    RETURN_WITH_STATUS(err, NULL);
  }

  nodeDestroy(root->rootNode, true, NULL);
  root->rootNode = node;
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
  //             "[INFO]: at pition %lu\n"
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
      nodeDestroy(left, true, NULL);
      return NULL;
    }
    TreeNode* right = nodeReadRecursion(vars, buf, bufSize, p, status, nodeCount);
    if (*status) {
      //Я знаю что тут всегда ноды и так нулевой указатель
      //однако на будущие случаи лучше иметь деструктор чем не иметь
      nodeDestroy(left, true, NULL);
      nodeDestroy(right, true, NULL);
      return NULL;
    }

    uint childN = (uint)(left != NULL) + (uint)(right != NULL);
    if (expectedChildN != childN) {
      nodeDestroy(left, true, NULL); 
      nodeDestroy(right, true, NULL);
      DUMP_ERROR_RETURN("Node has too few or too many null children!");
    }

    //fprintf(stderr, "Pos is %lu BufSize is %lu\n", *p, bufSize);
    SKIP_WHITESPACE;
    if (buf[*p] == ')') {
      (*p)++;
    } else {
      nodeDestroy(left, true, NULL);
      nodeDestroy(right, true, NULL);
      DUMP_ERROR_RETURN("No closing parenthesis");
    }

    TreeNode* node = nodeAlloc(data, NULL, left, right, status);
    if (*status) {
      nodeDelete(left, true, NULL);
      nodeDelete(right, true, NULL);
      nodeDestroy(node, true, NULL);
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

