#ifndef LEXER_H
#define LEXER_H

#include "ds/da/da.h"
#include "utils/utils.h"

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

extern const size_t TOKEN_TYPES_SIZE; 

const char* getTokenTypeStr(TokenType type);

typedef struct Token {
  TokenType type;
  int64_t value;
  size_t line;
  size_t lineStart;
  size_t pos;
} Token;

typedef DynamicArray Tokens;

typedef struct Lexer {
  Tokens tokens;
  MappedFile mf;
  size_t line;
  size_t lineStart;
  size_t pos;
} Lexer;


Lexer* lexerAlloc(int fd, size_t initCap, Error* status);
Error  lexerAnalyze(Lexer* lexer);
Error  lexerDestroy(Lexer* lexer);
Error  lexerVerify(Lexer* lexer);

#endif
