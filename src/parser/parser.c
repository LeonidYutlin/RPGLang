#include "parser/parser.h"

//TODO: add asserts in every static function
static bool getStatement(Tokens* t, size_t* i, char* buf, TreeNode** result);
static bool getStatementBlock(Tokens* t, size_t* i, char* buf, TreeNode** result);
static bool getConditionBlock(TokenType tokType, CtrlType ctrlType, 
                              Tokens* t, size_t* i, char* buf, TreeNode** result);
static bool getIf(Tokens* t, size_t* i, char* buf, TreeNode** result);
#define getWhile(t, i, buf, result) getConditionBlock(TOK_WHILE, CTRL_WHILE, t, i, buf, result)
#define getUntil(t, i, buf, result) getConditionBlock(TOK_UNTIL, CTRL_UNTIL, t, i, buf, result)
static bool getVariableDeclaration(Tokens* t, size_t* i, char* buf, TreeNode** result);
static bool getAssignment(Tokens* t, size_t* i, char* buf, TreeNode** result);
static bool getAssignmentBody(Tokens* t, size_t* i, TreeNode** result);
static bool getExpression(Tokens* t, size_t* i, TreeNode** result);
static bool getTerm(Tokens* t, size_t* i, TreeNode** result);
static bool getPrimary(Tokens* t, size_t* i, TreeNode** result);
static bool getNonVoidType(Tokens* t, size_t* i, TreeNode** result);
static bool getIdentifier(Tokens* t, size_t* i, char* buf, TreeNode** result);
static bool getNumber(Tokens* t, size_t* i, TreeNode** result);
static bool consumeToken(Tokens* t, size_t* i, TokenType type);

// this function isn't finalized. 
// represents Grammar in grammar.txt, which is currently Statement+, EOF
TreeNode* parse(Tokens* t, char* buf) {
  if (daVerify(t) || !buf)
    return NULL;

  size_t i = 0;
  TreeNode* firstStmt = NULL;
  if (!getStatement(t, &i, buf, &firstStmt)) {
    logln(ERROR, "Invalid Statement");
    return NULL;
  }
  if (!firstStmt) 
    firstStmt = SEMIC_(NULL);

  TreeNode* nextStmt = NULL;
  for (TreeNode* curStmt = firstStmt;
       getStatement(t, &i, buf, &nextStmt);) {
    if (!nextStmt)
      continue;
    TreeNode* lastStmt = curStmt;
    while (lastStmt->right)
      lastStmt = lastStmt->right;

    lastStmt->right = nextStmt;
    curStmt = nextStmt;
  }

  TokenType nextType = ((Token*)daGet(t, i++))->type; 
  if (nextType != TOK_EOF) {
    logln(ERROR, "Expected EOF, got %s", getTokenTypeStr(nextType));
    nodeDestroy(firstStmt);
    return NULL;
  }
  nodeFixParents(firstStmt);
  return firstStmt;
}

#define PEEK() ((Token*)daGet(t, (*i)))
#define CHECK(T) (PEEK()->type == T)

static bool getStatement(Tokens* t, size_t* i, char* buf, TreeNode** result) {
  if (consumeToken(t, i, TOK_SEMIC)) {
    *result = NULL;
    return true;
  }

  TreeNode* stmt = NULL;
  // statements that do not require a semicolon
  if (getStatementBlock(t, i, buf, &stmt) ||
      getIf(t, i, buf, &stmt)             ||
      getWhile(t, i, buf, &stmt)          ||
      getUntil(t, i, buf, &stmt)) {
    *result = stmt;
    return true;
  } 

  // statements that require a semicolon
  if (getExpression(t, i, &stmt)               ||
      getVariableDeclaration(t, i, buf, &stmt) ||
      getAssignment(t, i, buf, &stmt)) {
    if (!consumeToken(t, i, TOK_SEMIC)) {
      nodeDestroy(stmt);
      return false;
    }

    *result = SEMIC_(stmt);
    return true;
  }

  return false;
}

static bool getStatementBlock(Tokens* t, size_t* i, char* buf, TreeNode** result) {
  if (!consumeToken(t, i, TOK_LBRACE))
    return false;
  if (consumeToken(t, i, TOK_RBRACE)) {
    *result = NULL; // empty block "{}"
    return true;
  }

  TreeNode* firstStmt = NULL;
  if (!getStatement(t, i, buf, &firstStmt))
    return false;

  TreeNode* nextStmt = NULL;
  for (TreeNode* curStmt = firstStmt;
       getStatement(t, i, buf, &nextStmt);) {
    if (!nextStmt)
      continue;
    TreeNode* lastStmt = curStmt;
    while (lastStmt->right)
      lastStmt = lastStmt->right;

    lastStmt->right = nextStmt;
    curStmt = nextStmt;
  }

  if (!consumeToken(t, i, TOK_RBRACE)) {
    nodeDestroy(firstStmt);
    return false;
  }

  *result = firstStmt 
            ? SEMIC_(firstStmt)
            : NULL;
  return true;
}

static bool getConditionBlock(TokenType tokType, CtrlType ctrlType, 
                              Tokens* t, size_t* i, char* buf, TreeNode** result) {
  TreeNode* lhs = NULL;
  TreeNode* rhs = NULL;
  if (consumeToken(t, i, tokType)    &&
      consumeToken(t, i, TOK_LPAREN) &&
      getExpression(t, i, &lhs)      &&
      consumeToken(t, i, TOK_RPAREN) &&
      getStatement(t, i, buf, &rhs)) {
    *result = nodeAllocCtrl(ctrlType, lhs, rhs);
    return true;
  }

  nodeDestroy(lhs);
  nodeDestroy(rhs);
  return false;
}

static bool getIf(Tokens* t, size_t* i, char* buf, TreeNode** result) {
  TreeNode* ifNode   = NULL;
  TreeNode* elseStmt = NULL;
  if (getConditionBlock(TOK_IF, CTRL_IF, t, i, buf, &ifNode)) {
    if (consumeToken(t, i, TOK_ELSE)) {
      if (getStatement(t, i, buf, &elseStmt)) {
        TreeNode* lastStmt = ifNode;
        while (lastStmt->right)
          lastStmt = lastStmt->right;

        lastStmt->right = ELSE_(elseStmt);
      } else {
        nodeDestroy(ifNode);
        return false;
      }
    }

    *result = ifNode;
    return true;
  }

  nodeDestroy(ifNode);
  nodeDestroy(elseStmt);
  return false; 
}

static bool getVariableDeclaration(Tokens* t, size_t* i, char* buf, TreeNode** result) {
  TreeNode* type = NULL;
  TreeNode* lhs = NULL;
  if (getNonVoidType(t, i, &type) &&
      getIdentifier(t, i, buf, &lhs)) {
    TreeNode* rhs = NULL;
    if (getAssignmentBody(t, i, &rhs)) {
      *result = VAR_DECL_(type, ASG_(lhs, rhs));
      return true;
    }
    nodeDestroy(rhs);
    *result = VAR_DECL_(type, lhs);
    return true;
  }

  nodeDestroy(lhs);
  return false;
}

static bool getAssignment(Tokens* t, size_t* i, char* buf, TreeNode** result) {
  TreeNode* lhs = NULL;
  TreeNode* rhs = NULL;
  if (getIdentifier(t, i, buf, &lhs) &&
      getAssignmentBody(t, i, &rhs)) {
    *result = ASG_(lhs, rhs);
    return true;
  }

  nodeDestroy(lhs);
  nodeDestroy(rhs);
  return false;
}

static bool getAssignmentBody(Tokens* t, size_t* i, TreeNode** result) {
  TreeNode* expr = NULL;
  if (consumeToken(t, i, TOK_MIRROR) &&
      getExpression(t, i, &expr)) {
    *result = expr;
    return true;
  }

  nodeDestroy(expr);
  return false;
}

static bool getExpression(Tokens* t, size_t* i, TreeNode** result) {
  TreeNode* first = NULL;
  if (!getTerm(t, i, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_UNITE ||
       opTok->type == TOK_HIT;
       opTok = PEEK()) {
    (*i)++;
    TreeNode* next = NULL;
    if (!getTerm(t, i, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = opTok->type == TOK_UNITE
            ? ADD_(first, next)
            : SUB_(first, next);
  }
  *result = first;
  return true;
}

static bool getTerm(Tokens* t, size_t* i, TreeNode** result) {
  TreeNode* first = NULL;
  if (!getPrimary(t, i, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_EMPOWER ||
       opTok->type == TOK_SHATTER;
       opTok = PEEK()) {
    (*i)++;
    TreeNode* next = NULL;
    if (!getPrimary(t, i, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = opTok->type == TOK_EMPOWER
            ? MUL_(first, next)
            : DIV_(first, next);
  }
  *result = first;
  return true;
}

static bool getPrimary(Tokens* t, size_t* i, TreeNode** result) {
  if (consumeToken(t, i, TOK_LPAREN)) {
    TreeNode* expr = NULL;
    if (getExpression(t, i, &expr) &&
        consumeToken(t, i, TOK_RPAREN)) {
      *result = expr;
      return true;
    }

    nodeDestroy(expr);
    return false;
  }

  TreeNode* num = NULL;
  if (getNumber(t, i, &num)) {
    *result = num;
    return true;
  }

  return false;
}

static bool consumeToken(Tokens* t, size_t* i, TokenType type) {
  if (CHECK(type)) {
    (*i)++;
    return true;
  }
  return false;
}

static bool getIdentifier(Tokens* t, size_t* i, char* buf, TreeNode** result) {
  if (CHECK(TOK_IDENTIFIER)) {
    *result = VAR_(buf + PEEK()->pos, PEEK()->len);
    (*i)++;
    return true;
  }
  return false;
}

static bool getNumber(Tokens* t, size_t* i, TreeNode** result) {
  if (CHECK(TOK_NUM_LIT)) {
    *result = NUM_(PEEK()->value);
    (*i)++;
    return true;
  }
  return false;
}

static bool getNonVoidType(Tokens* t, size_t* i, TreeNode** result) {
  switch (PEEK()->type) {
    case TOK_PRIM: *result = PRIM_(); (*i)++; return true;
    case TOK_FRAC: *result = FRAC_(); (*i)++; return true;
    case TOK_LOC:  *result = LOC_();  (*i)++; return true;
    default: return false;
  }
}
