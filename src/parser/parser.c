#include "parser/parser.h"

static TreeNode* getN(Tokens* t, size_t* i);
static TreeNode* getE(Tokens* t, size_t* i);
static TreeNode* getT(Tokens* t, size_t* i);
static TreeNode* getP(Tokens* t, size_t* i);

TreeNode* parse(Tokens* t) {
  if (daVerify(t))
    return NULL;
 
  size_t i = 0;
  TreeNode* val = getE(t, &i);
  if (!val) {
    logln(ERROR, "Expected expr");
    return NULL;
  }

  if (((Token*)daGet(t, i++))->type != TOK_EOF) {
    logln(ERROR, "Expected EOF");
    nodeDestroy(val);
    return NULL;
  }
  nodeFixParents(val);
  return val;
}

#define NEXT() ((Token*)daGet(t, (*i)))

static TreeNode* getN(Tokens* t, size_t* i) {
  logln(INFO, "Number parsing!");
  Token* next = NEXT();
  if (next->type == TOK_NUM_LIT) {
    (*i)++;
    logln(INFO, "we are on the %zu elem out of %zu elems", *i, t->count);
    return NUM_(next->value);
  }
  return NULL;
}

static TreeNode* getE(Tokens* t, size_t* i) {
  logln(INFO, "Expression parsing!");
  TreeNode* val = getT(t, i);
  for (Token* next = NEXT(); 
       next->type == TOK_UNITE ||
       next->type == TOK_HIT;
       next = NEXT()) {
    logln(INFO, "Next is %s", getTokenTypeStr(next->type));
    (*i)++;
    logln(INFO, "we are on the %zu elem out of %zu elems", *i, t->count);
    TreeNode* val2 = getT(t, i);
    if (next->type == TOK_UNITE)
      val = ADD_(val, val2);
    else
      val = SUB_(val, val2);
  }
  return val;
}

static TreeNode* getT(Tokens* t, size_t* i) {
  logln(INFO, "Term parsing!");
  TreeNode* val = getP(t, i);
  for (Token* next = NEXT(); 
       next->type == TOK_EMPOWER ||
       next->type == TOK_SHATTER;
       next = NEXT()) {
    logln(INFO, "Next is %s", getTokenTypeStr(next->type));
    (*i)++;
    logln(INFO, "we are on the %zu elem out of %zu elems", *i, t->count);
    TreeNode* val2 = getP(t, i);
    if (next->type == TOK_EMPOWER)
      val = MUL_(val, val2);
    else
      val = DIV_(val, val2);
  }
  return val;
}

static TreeNode* getP(Tokens* t, size_t* i) {
  for (Token* next = NEXT();
       next->type == TOK_LPAREN;
       next = NEXT()) {
    (*i)++;
    TreeNode* val = getE(t, i);
    next = NEXT();
    if (next->type == TOK_RPAREN) {
      (*i)++;
      return val;
    } else {
      logln(ERROR, "Expected a closing parenthethis");
      nodeDestroy(val);
      return NULL;
    }
  }
  return getN(t, i);
}
