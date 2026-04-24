#include "parser/parser.h"

//TODO: add asserts in every static function
static TreeNode* getStatement(Tokens* t, size_t* i, char* buf);
static TreeNode* getStatementBlock(Tokens* t, size_t* i, char* buf);
static TreeNode* getIf(Tokens* t, size_t* i, char* buf);
static TreeNode* getAssignment(Tokens* t, size_t* i, char* buf);
static TreeNode* getExpression(Tokens* t, size_t* i);
static TreeNode* getTerm(Tokens* t, size_t* i);
static TreeNode* getPrimary(Tokens* t, size_t* i);

TreeNode* parse(Tokens* t, char* buf) {
  if (daVerify(t) || !buf)
    return NULL;

  size_t i = 0;
  TreeNode* val = getStatement(t, &i, buf);
  if (!val) {
    logln(ERROR, "Invalid Statement");
    return NULL;
  }

  TreeNode* nextStmt = getStatement(t, &i, buf);
  for (TreeNode* curStmt = val; 
       nextStmt; 
       nextStmt = getStatement(t, &i, buf)) {
    TreeNode* lastStmt = curStmt;
    while (OF_CTRL(lastStmt, CTRL_IF)) {
      lastStmt = lastStmt->right;
    }
    lastStmt->right = nextStmt;
    curStmt = nextStmt;
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

static TreeNode* getStatement(Tokens* t, size_t* i, char* buf) {
  if (CHECK(TOK_SEMIC)) {
    (*i)++;
    return SEMIC_(NULL);
  }

  bool requiresSemicolon = false;
  TreeNode* stmt = getStatementBlock(t, i, buf);
  if (!stmt)
    stmt = getIf(t, i, buf);
  if (!stmt) {
    requiresSemicolon = true;
    stmt = getExpression(t, i);
  }
  if (!stmt)
    stmt = getAssignment(t, i, buf);

  if (!stmt)
    return NULL;

  if (!requiresSemicolon)
    return stmt;

  if (!CHECK(TOK_SEMIC)) {
    nodeDestroy(stmt);
    return NULL;
  }
  (*i)++;

  return SEMIC_(stmt);
}

static TreeNode* getStatementBlock(Tokens* t, size_t* i, char* buf) {
  logln(DEBUG, "Stmt block parsing!");
  if (!CHECK(TOK_LBRACE))
    return NULL;
  (*i)++;
  if (CHECK(TOK_RBRACE)) {
    (*i)++;
    return SEMIC_(NULL);
  }

  TreeNode* firstStmt = getStatement(t, i, buf);
  if (!firstStmt) {
    return NULL;
  }

  TreeNode* nextStmt = getStatement(t, i, buf);
  for (TreeNode* curStmt = firstStmt; 
       nextStmt;
       nextStmt = getStatement(t, i, buf)) {
    TreeNode* lastStmt = curStmt;
    while (OF_CTRL(lastStmt, CTRL_IF)) {
      lastStmt = lastStmt->right;
    }
    lastStmt->right = nextStmt;
    curStmt = nextStmt;
  }

  if (!CHECK(TOK_RBRACE)) {
    nodeDestroy(firstStmt);
    return NULL;
  }
  (*i)++;

  return SEMIC_(firstStmt);
}

static TreeNode* getIf(Tokens* t, size_t* i, char* buf) {
  logln(DEBUG, "If parsing!");
  if (!CHECK(TOK_IF))
    return NULL;
  (*i)++;
  if (!CHECK(TOK_LPAREN))
    return NULL;
  (*i)++;
  TreeNode* lhs = getExpression(t, i);
  if (!lhs)
    return NULL;

  if (!CHECK(TOK_RPAREN)) {
    nodeDestroy(lhs);
    return NULL;
  }
  (*i)++;

  TreeNode* rhs = getStatement(t, i, buf);
  if (!rhs) {
    nodeDestroy(lhs);
    return NULL;
  }
  return IF_(lhs, rhs);
}

static TreeNode* getAssignment(Tokens* t, size_t* i, char* buf) {
  logln(DEBUG, "Assignment parsing!");
  TreeNode* lhs = NULL;
  if (CHECK(TOK_IDENTIFIER)) {
    lhs = VAR_(buf + PEEK()->pos, PEEK()->len);
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
  logln(DEBUG, "Expression parsing!");
  TreeNode* val = getTerm(t, i);
  for (Token* next = PEEK(); 
       next->type == TOK_UNITE ||
       next->type == TOK_HIT;
       next = PEEK()) {
    logln(DEBUG, "Next is %s", getTokenTypeStr(next->type));
    (*i)++;
    logln(DEBUG, "we are on the %zu elem out of %zu elems", *i, t->count);
    TreeNode* val2 = getTerm(t, i);
    if (next->type == TOK_UNITE)
      val = ADD_(val, val2);
    else
      val = SUB_(val, val2);
  }
  return val;
}

static TreeNode* getTerm(Tokens* t, size_t* i) {
  logln(DEBUG, "Term parsing!");
  TreeNode* val = getPrimary(t, i);
  for (Token* next = PEEK(); 
       next->type == TOK_EMPOWER ||
       next->type == TOK_SHATTER;
       next = PEEK()) {
    logln(DEBUG, "Next is %s", getTokenTypeStr(next->type));
    (*i)++;
    logln(DEBUG, "we are on the %zu elem out of %zu elems", *i, t->count);
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
