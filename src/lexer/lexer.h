#ifndef LEXER_H
#define LEXER_H

#include "ds/da/da.h"

#define TOKEN_TYPE_LIST() \
  X(TOK_EOF,        "EOF") \
  X(TOK_IDENTIFIER, "IDENTIFIER") \
  X(TOK_LPAR,       "'('") \
  X(TOK_RPAR,       "')'") \
  X(TOK_NUM_LIT,    "NUM_LITERAL")

typedef enum TokenType {
  #define X(enm, ...) enm,
  TOKEN_TYPE_LIST()
  #undef X
} TokenType;

typedef struct Token {
  TokenType type;
  size_t line;
  size_t lineStart;
  size_t pos;
} Token;

typedef DynamicArray Tokens;

typedef struct Lexer {
  Tokens tokens;
  char* buf;
  size_t bufSize;
  size_t line;
  size_t lineStart;
  size_t pos;
} Lexer;

Lexer* lexerAlloc(FILE* sourceFile, size_t initialCapacity, Error* status);
Error  lexerAnalyze(Lexer* lexer);
Error  lexerDestroy(Lexer* lexer);
Error  lexerVerify(Lexer* lexer);

#endif
