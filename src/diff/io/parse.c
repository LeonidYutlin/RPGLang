#include "diff/io/parse.h"
#include "misc/util.h"
#include <ctype.h>

static TreeNode* getE(const char* buf, size_t* p, Variables* vars);
static TreeNode* getT(const char* buf, size_t* p, Variables* vars);
static TreeNode* getP(const char* buf, size_t* p, Variables* vars);
static TreeNode* getN(const char* buf, size_t* p, Variables* vars);
static TreeNode* getV(const char* buf, size_t* p, Variables* vars);

static bool isvar(const char c);

#define SYNTAX_ERROR(commentary, expectedCharStr, p)                \
  {                                                                 \
  fprintf(stderr,                                                   \
          "[ERROR]: Failed to read given mathematical expression\n" \
          "\tAt position %lu\n"                                     \
          "\t%s\n"                                                  \
          "\tExpected char: %s\n"                                   \
          "\tString snippet:\n"                                     \
          "\t%.10s...\n"                                            \
          "\t^\n",                                                  \
          p,                                                        \
          commentary,                                               \
          expectedCharStr,                                          \
          buf + p);                                                 \
  return NULL;                                                      \
  }

#define SKIP_WHITESPACE(a)    \
  while (isspace(buf[(a)])) { \
      (a)++;                  \
  }

TreeNode* parseFormula(const char* buf, Variables* vars) {
  if (!buf ||
      !vars)
    return NULL;

  size_t p = 0;
  TreeNode* val = getE(buf, &p, vars);
  SKIP_WHITESPACE(p);
  if (buf[p] != '\0')
    SYNTAX_ERROR("Illegal character at the end of given expression", "NULL character ('\\0')", p);
  p++;
  nodeFixParents(val);
  return val;
}

static TreeNode* getE(const char* buf, size_t* p, Variables* vars) {
  SKIP_WHITESPACE(*p);
  TreeNode* val = getT(buf, p, vars);
  while (buf[*p] == '+' ||
         buf[*p] == '-') {
    char op = buf[*p];
    (*p)++;
    TreeNode* val2 = getT(buf, p, vars);
    if (op == '+')
      val = ADD_(val, val2);
    else
      val = SUB_(val, val2);
  }
  SKIP_WHITESPACE(*p);
  return val;
}

static TreeNode* getT(const char* buf, size_t* p, Variables* vars) {
  SKIP_WHITESPACE(*p);
  TreeNode* val = getP(buf, p, vars);
  while (buf[*p] == '*' ||
         buf[*p] == '/') {
    char op = buf[*p];
    (*p)++;
    TreeNode* val2 = getP(buf, p, vars);
    if (op == '*')
      val = MUL_(val, val2);
    else
      val = DIV_(val, val2);
  }
  SKIP_WHITESPACE(*p);
  return val;
}

static TreeNode* getP(const char* buf, size_t* p, Variables* vars) {
  SKIP_WHITESPACE(*p);
  while (buf[*p] == '(') {
    (*p)++;
    SKIP_WHITESPACE(*p);
    TreeNode* val = getE(buf, p, vars);
    SKIP_WHITESPACE(*p);
    if (buf[*p] == ')')
      (*p)++;
    else
      SYNTAX_ERROR("Illegal character at the end of a primary expression", ")", *p);
    return val;
  }
  SKIP_WHITESPACE(*p);
  TreeNode* res = getV(buf, p, vars); 
  return res 
         ? res 
         : getN(buf, p, vars);
}

static TreeNode* getN(const char* buf, size_t* p, unused Variables* vars) {
  SKIP_WHITESPACE(*p);
  bool neg = false;
  double val = 0;
  size_t oldP = *p;
  if (buf[*p] == '-') {
    neg = true;
    (*p)++;
  }     
  while (isdigit(buf[*p])) {
      val = val * 10 + (buf[*p] - '0');
      (*p)++;
  }
  if (neg) 
    val *= -1.0f;
  if (oldP == *p)
    SYNTAX_ERROR("Illegal char at the start of a number", "[0-9, -]", *p);
  SKIP_WHITESPACE(*p);
  return NUM_(val);
}

static TreeNode* getV(const char* buf, size_t* p, Variables* vars) {
  SKIP_WHITESPACE(*p);
  size_t oldP = *p;
  char varName[MAX_VALUE_STRING_LENGTH] = {0};
  for (size_t i = 0;
       ((oldP == *p && 
        !isdigit(buf[*p])) ||
        oldP != *p)    &&
        isvar(buf[*p]) &&
        i < MAX_VALUE_STRING_LENGTH;
       i++) {
    varName[i] = buf[*p];
    (*p)++;
  }
  if (oldP == *p)
    return NULL;
  varName[MAX_VALUE_STRING_LENGTH - 1] = '\0'; //ensure it is NULL-terminated
  //int opType = getOpType(varName);
  //if (opType >= 0) {
    //return op type with args...
  //}
  Error err = OK;
  size_t index = regVar(vars, varName, &err);
  if (err != OK &&
      err != AttemptedReregistration) {
    prettyError(stderr, err);
    return NULL;
  }
  SKIP_WHITESPACE(*p);
  return VAR_(index);
}

static bool isvar(const char c) {
  return isalnum(c) ||
         c == '\\'  ||
         c == '_';
}

#undef SYNTAX_ERROR
#undef SKIP_WHITESPACE
