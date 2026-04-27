#ifndef LEXER_H
#define LEXER_H

#include "ds/da/da.h"
#include "utils/utils.h"
#include <stdint.h>

#define KEYWORD_LIST()           \
  X(TOK_MAGE, "#mage")           \
  X(TOK_WARRIOR, "#warrior")     \
  X(TOK_PRIEST, "#priest")       \
  X(TOK_WARLOCK, "#warlock")     \
  X(TOK_ENCRYPT, "encrypt")      \
  X(TOK_FREEZE, "freeze")        \
  X(TOK_SCRY, "scry")            \
  X(TOK_DELAYED, "delayed")      \
  X(TOK_PREPARE, "prepare")      \
  X(TOK_EQUIP, "equip")          \
  X(TOK_MIRROR, "mirror")        \
  X(TOK_CONJURE, "conjure")      \
  X(TOK_SLICE, "slice")          \
  X(TOK_SLAM, "slam")            \
  X(TOK_HIT, "hit")              \
  X(TOK_DUELL, "duel!<")         \
  X(TOK_DUELR, "duel!>")         \
  X(TOK_DUELLR, "duel!<>")       \
  X(TOK_WORTHY, "worthy")        \
  X(TOK_TRAIN, "train")          \
  X(TOK_PUSHL, "<push")          \
  X(TOK_PUSHR, "push>")          \
  X(TOK_PURIFY, "purify")        \
  X(TOK_SHADOW, "shadow")        \
  X(TOK_ENGLIGHTEN, "enlighten") \
  X(TOK_UNTIL, "until")          \
  X(TOK_WHILE, "while")          \
  X(TOK_IF, "if")                \
  X(TOK_ELSE, "else")            \
  X(TOK_PENANCE, "penance")      \
  X(TOK_UNITE, "unite")          \
  X(TOK_AND, "and")              \
  X(TOK_NOT, "not")              \
  X(TOK_BLIGHTEN, "blighten")    \
  X(TOK_SHATTER, "shatter")      \
  X(TOK_LEECH, "leech")          \
  X(TOK_SACRIFICE, "sacrifice")  \
  X(TOK_OR, "or")                \
  X(TOK_ROOTS, "roots")          \
  X(TOK_NAT_AGE, "natage")       \
  X(TOK_NAT_GROWTH, "natgrowth") \
  X(TOK_EMPOWER, "empower")      \
  X(TOK_SYMBIOSIS, "symbiosis")  \
  X(TOK_COMPLETE, "complete")    \
  X(TOK_ROLLBACK, "rollback")    \
  X(TOK_SKIP, "skip")            \
  X(TOK_PRIM, "prim")            \
  X(TOK_FRAC, "frac")            \
  X(TOK_LOC,  "loc")             \
  X(TOK_VOID, "void")

#define KEYWORD_ALIAS_LIST() \
  X(TOK_PRIM, "primordial")  \
  X(TOK_FRAC, "fractured")   \
  X(TOK_LOC,  "location")

#define TOKEN_TYPE_LIST()          \
  X(TOK_EOF,        "EOF")         \
  X(TOK_IDENTIFIER, "IDENTIFIER")  \
  X(TOK_SEMIC,      "';'")         \
  X(TOK_LPAREN,       "'('")       \
  X(TOK_RPAREN,       "')'")       \
  X(TOK_LBRACE,     "'{'")         \
  X(TOK_RBRACE,     "'}'")         \
  X(TOK_NUM_LIT,    "NUM_LITERAL") \
  KEYWORD_LIST()

typedef enum TokenType {
  #define X(enm, ...) enm,
  TOKEN_TYPE_LIST()
  #undef X
} TokenType;

extern const size_t TOKEN_TYPES_SIZE; 

const char* getTokenTypeStr(TokenType type);

// TODO: add const qualifiers to char* here
typedef struct Token {
  TokenType type;
  uint64_t value;
  char* line;
  char* lineStart;
  char* pos;
  size_t len;
} Token;

typedef DynamicArray Tokens;

typedef struct Lexer {
  Tokens tokens;
  MappedFile mf;
  char* line;
  char* lineStart;
  size_t pos;
} Lexer;

Error  lexerInit(Lexer* lexer, int fd, size_t initCap);
Lexer* lexerAlloc(int fd, size_t initCap, Error* status);
Error  lexerAnalyze(Lexer* lexer);
Error  lexerDestroy(Lexer* lexer, bool isAlloced);
Error  lexerPrintTokens(FILE* sink, Lexer* lexer);
Error  lexerVerify(Lexer* lexer);

#endif
