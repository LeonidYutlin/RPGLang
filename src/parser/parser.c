#include "parser/parser.h"

static TreeNode* getStatement(Tokens* t, size_t* i);
static TreeNode* getAssignment(Tokens* t, size_t* i);
static TreeNode* getExpression(Tokens* t, size_t* i);
static TreeNode* getTerm(Tokens* t, size_t* i);
static TreeNode* getPrimary(Tokens* t, size_t* i);

TreeNode* parse(Tokens* t) {
  if (daVerify(t))
    return NULL;
 
  size_t i = 0;
  TreeNode* val = getStatement(t, &i);
  if (!val) {
    logln(ERROR, "Invalid Statement");
    return NULL;
  }

  TokenType nextType = ((Token*)daGet(t, i++))->type; 
  if (nextType != TOK_EOF) {
    logln(ERROR, "Expected EOF, got %s", getTokenTypeStr(nextType));
    nodeDestroy(val);
    return NULL;
  }
  nodeFixParents(val);
  return val;
}

#define PEEK() ((Token*)daGet(t, (*i)))
#define CHECK(T) (PEEK()->type == T)

static TreeNode* getStatement(Tokens* t, size_t* i) {
  TreeNode* stmt = NULL;
  if ((stmt = getExpression(t, i))) {
    return stmt;
  }

  stmt = getAssignment(t, i);
  if (!stmt)
    return NULL;

  if (!CHECK(TOK_SEMIC)) {
    nodeDestroy(stmt);
    return NULL;
  }
  (*i)++;

  return stmt;
}

static TreeNode* getAssignment(Tokens* t, size_t* i) {
  logln(INFO, "Assignment parsing!");
  TreeNode* lhs = NULL;
  if (CHECK(TOK_IDENTIFIER)) {
    lhs = NUM_(PEEK()->value);
    (*i)++;
  } else
    return NULL;

  if (!CHECK(TOK_MIRROR)) {
    nodeDestroy(lhs);
    return NULL;
  }
  (*i)++;

  TreeNode* rhs = getExpression(t, i);
  if (!rhs) {
    nodeDestroy(lhs);
    return NULL;
  }
  return ASG_(lhs, rhs);
}

static TreeNode* getExpression(Tokens* t, size_t* i) {
  logln(INFO, "Expression parsing!");
  TreeNode* val = getTerm(t, i);
  for (Token* next = PEEK(); 
       next->type == TOK_UNITE ||
       next->type == TOK_HIT;
       next = PEEK()) {
    logln(INFO, "Next is %s", getTokenTypeStr(next->type));
    (*i)++;
    logln(INFO, "we are on the %zu elem out of %zu elems", *i, t->count);
    TreeNode* val2 = getTerm(t, i);
    if (next->type == TOK_UNITE)
      val = ADD_(val, val2);
    else
      val = SUB_(val, val2);
  }
  return val;
}

static TreeNode* getTerm(Tokens* t, size_t* i) {
  logln(INFO, "Term parsing!");
  TreeNode* val = getPrimary(t, i);
  for (Token* next = PEEK(); 
       next->type == TOK_EMPOWER ||
       next->type == TOK_SHATTER;
       next = PEEK()) {
    logln(INFO, "Next is %s", getTokenTypeStr(next->type));
    (*i)++;
    logln(INFO, "we are on the %zu elem out of %zu elems", *i, t->count);
    TreeNode* val2 = getPrimary(t, i);
    if (next->type == TOK_EMPOWER)
      val = MUL_(val, val2);
    else
      val = DIV_(val, val2);
  }
  return val;
}

static TreeNode* getPrimary(Tokens* t, size_t* i) {
  if (CHECK(TOK_LPAREN)) {
    (*i)++;
    TreeNode* val = getExpression(t, i);
    if (val && CHECK(TOK_RPAREN)) {
      (*i)++;
      return val;
    } else {
      nodeDestroy(val);
      return NULL;
    }
  }

  if (CHECK(TOK_NUM_LIT)) {
    TreeNode* num = NUM_(PEEK()->value);
    (*i)++;
    return num;
  }

  return NULL;
}
