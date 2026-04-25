#include "parser/parser.h"

typedef struct {
  Tokens* t;
  size_t  i;
} Parser;

//TODO: add asserts in every static function
static bool getStatement(Parser* p, TreeNode** result);
static bool getStatementBlock(Parser* p, TreeNode** result);
static bool getConditionBlock(TokenType tokType, CtrlType ctrlType, 
                              Parser* p, TreeNode** result);
static bool getIf(Parser* p, TreeNode** result);
#define getWhile(p, result) getConditionBlock(TOK_WHILE, CTRL_WHILE, p, result)
#define getUntil(p, result) getConditionBlock(TOK_UNTIL, CTRL_UNTIL, p, result)
static bool getVariableDeclaration(Parser* p, TreeNode** result);
static bool getAssignment(Parser* p, TreeNode** result);
static bool getAssignmentBody(Parser* p, TreeNode** result);
static bool getExpression(Parser* p, TreeNode** result);
static bool getTerm(Parser* p, TreeNode** result);
static bool getPrimary(Parser* p, TreeNode** result);
static bool getNonVoidType(Parser* p, TreeNode** result);
static bool getIdentifier(Parser* p, TreeNode** result);
static bool getNumber(Parser* p, TreeNode** result);
static bool consumeToken(Parser* p, TokenType type);

// this function isn't finalized. 
// represents Grammar in grammar.txt, which is currently Statement+, EOF
TreeNode* parse(Tokens* t) {
  if (daVerify(t))
    return NULL;

  Parser p = (Parser){ .t = t, .i = 0 };
  TreeNode* firstStmt = NULL;
  if (!getStatement(&p, &firstStmt)) {
    logln(ERROR, "Invalid Statement");
    return NULL;
  }
  if (!firstStmt) 
    firstStmt = SEMIC_(NULL);

  TreeNode* nextStmt = NULL;
  for (TreeNode* curStmt = firstStmt;
       getStatement(&p, &nextStmt);) {
    if (!nextStmt)
      continue;
    TreeNode* lastStmt = curStmt;
    while (lastStmt->right)
      lastStmt = lastStmt->right;

    lastStmt->right = nextStmt;
    curStmt = nextStmt;
  }

  TokenType nextType = ((Token*)daGet(t, p.i++))->type; 
  if (nextType != TOK_EOF) {
    logln(ERROR, "Expected EOF, got %s", getTokenTypeStr(nextType));
    nodeDestroy(firstStmt);
    return NULL;
  }
  nodeFixParents(firstStmt);
  return firstStmt;
}

#define PEEK() ((Token*)daGet(p->t, p->i))
#define CHECK(T) (PEEK()->type == T)

static bool getStatement(Parser* p, TreeNode** result) {
  if (consumeToken(p, TOK_SEMIC)) {
    *result = NULL;
    return true;
  }

  TreeNode* stmt = NULL;
  // statements that do not require a semicolon
  if (getStatementBlock(p, &stmt) ||
      getIf(p, &stmt)             ||
      getWhile(p, &stmt)          ||
      getUntil(p, &stmt)) {
    *result = stmt;
    return true;
  } 

  // statements that require a semicolon
  if (getExpression(p, &stmt)               ||
      getVariableDeclaration(p, &stmt) ||
      getAssignment(p, &stmt)) {
    if (!consumeToken(p, TOK_SEMIC)) {
      nodeDestroy(stmt);
      return false;
    }

    *result = SEMIC_(stmt);
    return true;
  }

  return false;
}

static bool getStatementBlock(Parser* p, TreeNode** result) {
  if (!consumeToken(p, TOK_LBRACE))
    return false;
  if (consumeToken(p, TOK_RBRACE)) {
    *result = NULL; // empty block "{}"
    return true;
  }

  TreeNode* firstStmt = NULL;
  if (!getStatement(p, &firstStmt))
    return false;

  TreeNode* nextStmt = NULL;
  for (TreeNode* curStmt = firstStmt;
       getStatement(p, &nextStmt);) {
    if (!nextStmt)
      continue;
    TreeNode* lastStmt = curStmt;
    while (lastStmt->right)
      lastStmt = lastStmt->right;

    lastStmt->right = nextStmt;
    curStmt = nextStmt;
  }

  if (!consumeToken(p, TOK_RBRACE)) {
    nodeDestroy(firstStmt);
    return false;
  }

  *result = firstStmt 
            ? SEMIC_(firstStmt)
            : NULL;
  return true;
}

static bool getConditionBlock(TokenType tokType, CtrlType ctrlType, 
                              Parser* p, TreeNode** result) {
  TreeNode* lhs = NULL;
  TreeNode* rhs = NULL;
  if (consumeToken(p, tokType)    &&
      consumeToken(p, TOK_LPAREN) &&
      getExpression(p, &lhs)      &&
      consumeToken(p, TOK_RPAREN) &&
      getStatement(p, &rhs)) {
    *result = nodeAllocCtrl(ctrlType, lhs, rhs);
    return true;
  }

  nodeDestroy(lhs);
  nodeDestroy(rhs);
  return false;
}

static bool getIf(Parser* p, TreeNode** result) {
  TreeNode* ifNode   = NULL;
  TreeNode* elseStmt = NULL;
  if (getConditionBlock(TOK_IF, CTRL_IF, p, &ifNode)) {
    if (consumeToken(p, TOK_ELSE)) {
      if (getStatement(p, &elseStmt)) {
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

static bool getVariableDeclaration(Parser* p, TreeNode** result) {
  TreeNode* type = NULL;
  TreeNode* lhs = NULL;
  if (getNonVoidType(p, &type) &&
      getIdentifier(p, &lhs)) {
    TreeNode* rhs = NULL;
    if (getAssignmentBody(p, &rhs)) {
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

static bool getAssignment(Parser* p, TreeNode** result) {
  TreeNode* lhs = NULL;
  TreeNode* rhs = NULL;
  if (getIdentifier(p, &lhs) &&
      getAssignmentBody(p, &rhs)) {
    *result = ASG_(lhs, rhs);
    return true;
  }

  nodeDestroy(lhs);
  nodeDestroy(rhs);
  return false;
}

static bool getAssignmentBody(Parser* p, TreeNode** result) {
  TreeNode* expr = NULL;
  if (consumeToken(p, TOK_MIRROR) &&
      getExpression(p, &expr)) {
    *result = expr;
    return true;
  }

  nodeDestroy(expr);
  return false;
}

static bool getExpression(Parser* p, TreeNode** result) {
  TreeNode* first = NULL;
  if (!getTerm(p, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_UNITE ||
       opTok->type == TOK_HIT;
       opTok = PEEK()) {
    p->i++;
    TreeNode* next = NULL;
    if (!getTerm(p, &next)) {
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

static bool getTerm(Parser* p, TreeNode** result) {
  TreeNode* first = NULL;
  if (!getPrimary(p, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_EMPOWER ||
       opTok->type == TOK_SHATTER;
       opTok = PEEK()) {
    p->i++;
    TreeNode* next = NULL;
    if (!getPrimary(p, &next)) {
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

static bool getPrimary(Parser* p, TreeNode** result) {
  if (consumeToken(p, TOK_LPAREN)) {
    TreeNode* expr = NULL;
    if (getExpression(p, &expr) &&
        consumeToken(p, TOK_RPAREN)) {
      *result = expr;
      return true;
    }

    nodeDestroy(expr);
    return false;
  }

  TreeNode* prim = NULL;
  if (getNumber(p, &prim) ||
      getIdentifier(p, &prim)) {
    *result = prim;
    return true;
  }

  nodeDestroy(prim);
  return false;
}

static bool consumeToken(Parser* p, TokenType type) {
  if (CHECK(type)) {
    p->i++;
    return true;
  }
  return false;
}

static bool getIdentifier(Parser* p, TreeNode** result) {
  if (CHECK(TOK_IDENTIFIER)) {
    *result = VAR_(PEEK()->pos, PEEK()->len);
    p->i++;
    return true;
  }
  return false;
}

static bool getNumber(Parser* p, TreeNode** result) {
  if (CHECK(TOK_NUM_LIT)) {
    *result = NUM_(PEEK()->value);
    p->i++;
    return true;
  }
  return false;
}

static bool getNonVoidType(Parser* p, TreeNode** result) {
  switch (PEEK()->type) {
    case TOK_PRIM: *result = PRIM_(); p->i++; return true;
    case TOK_FRAC: *result = FRAC_(); p->i++; return true;
    case TOK_LOC:  *result = LOC_();  p->i++; return true;
    default: return false;
  }
}
