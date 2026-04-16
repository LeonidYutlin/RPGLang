#include "lexer/lexer.h"
#include "logger/logger.h"
#include "utils/utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char* const TOKEN_TYPES[] = {
  #define X(enm, str) \
    [enm] = str,

  TOKEN_TYPE_LIST()
  #undef X
};

const size_t TOKEN_TYPES_SIZE = sizer(TOKEN_TYPES);

static bool isIn(char c, const char* str);

Lexer* lexerAlloc(int fd, size_t initCap, Error* status) {
  if (fd < 0 || 
      !initCap)
    RETURN_WITH_STATUS(BadArgs, NULL);

  Lexer* lexer = (Lexer*)calloc(1, sizeof(Lexer));
  if (!lexer)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = daInit(&lexer->tokens, initCap, sizeof(Token), NULL))) {
    free(lexer);
    RETURN_WITH_STATUS(err, NULL);
  }

  if ((err = mappedFileInit(fd, &lexer->mf))) {
    free(lexer);
    RETURN_WITH_STATUS(err, NULL);
  }

  lexer->pos        = 0;
  lexer->line       = 1;
  lexer->lineStart  = 1;
  return lexer;
}

#define EMIT(T, position, length, ...) \
  {                                    \
    newToken = (Token){                \
      __VA_ARGS__ __VA_OPT__(,)        \
      .type = T,                       \
      .pos = position,                 \
      .len = length,                   \
      .line = lexer->line,             \
      .lineStart = lexer->lineStart,   \
    };                                 \
    daAppend(tokens, &newToken);       \
  }

#define CONSUME_CHAR(T, ...)        \
 {                                  \
    EMIT(T, lexer->pos, 1           \
        __VA_OPT__(,) __VA_ARGS__); \
    lexer->pos++;                   \
 }

static const char* const RESERVED_SPECIAL_CHARACTERS = ";(){}";
static const char* const ROMAN_NUMERALS = "IVXLC";

Error lexerAnalyze(Lexer* lexer) {
  Error err = OK;
  if ((err = lexerVerify(lexer)))
    return err;
  
  const char* buf = lexer->mf.data;
  size_t bufSize = lexer->mf.size;
  Tokens* tokens = &lexer->tokens;
  Token newToken = {};
  for (char c = buf[lexer->pos]; lexer->pos < bufSize; c = buf[lexer->pos]) {
    //skip whitespace
    if (isspace(c)) {
      if (c == '\n') {
        lexer->line++;
        lexer->lineStart = lexer->pos;
      }
      lexer->pos++;
      continue;
    }
    
    // one-character long tokens
    switch (c) {
      case ';': CONSUME_CHAR(TOK_SEMIC);  continue;
      case '(': CONSUME_CHAR(TOK_LPAR);   continue;
      case ')': CONSUME_CHAR(TOK_RPAR);   continue;
      case '{': CONSUME_CHAR(TOK_LBRACE); continue;
      case '}': CONSUME_CHAR(TOK_RBRACE); continue;
      default: break;
    }

    // numeric literals
    if (c == '0') {
      uint64_t num = 0;
      size_t oldPos = lexer->pos;
      lexer->pos++;
      if (lexer->pos < bufSize &&
          isIn(c = buf[lexer->pos], ROMAN_NUMERALS)) {
        uint64_t degree = 0;
        uint64_t newDegree = 0;
        uint64_t numBuf = 0;
        bool cont = true;
        while (cont && lexer->pos < bufSize) {

          c = buf[lexer->pos];
          switch (c) {
            case 'I': newDegree = 1;  break;
            case 'V': newDegree = 5;  break;
            case 'X': newDegree = 10; break;
            case 'L': newDegree = 50; break;
            case 'C': newDegree = 100; break;
            default:  cont = false; num += numBuf; continue;
          }

          if (degree && 
              degree != newDegree) {
            if (degree < newDegree) {
              num -= numBuf;
            } else {
              num += numBuf;
            }
            numBuf = 0;
          }

          degree = newDegree;
          numBuf += degree;

          lexer->pos++;
        }
      }
      EMIT(TOK_NUM_LIT, 
           oldPos,
           lexer->pos - oldPos,
           .value = num);
      continue;
    }

    // identifiers
    size_t oldPos = lexer->pos;
    lexer->pos++;
    while (lexer->pos < bufSize &&
           !isspace(c = buf[lexer->pos]) &&
           !isIn(c, RESERVED_SPECIAL_CHARACTERS)) {
      lexer->pos++;
    }

    EMIT(TOK_IDENTIFIER, 
         oldPos,
         lexer->pos - oldPos);
    continue;

    // should be unreachable
    logln(ERROR, "Unknown character '%c'. Skipping...", c);
    lexer->pos++;
  }
  EMIT(TOK_EOF, lexer->pos, 0);
  return OK;
}

#undef EMIT
#undef CONSUME_CHAR

Error lexerDestroy(Lexer* lexer) {
  if (!lexer)
    return BadArgs;
  
  daDestroy(&lexer->tokens, false);
  if (lexer->mf.data)
    mappedFileDestroy(&lexer->mf);
    
  free(lexer);

  return OK;
}

Error lexerVerify(Lexer* lexer) {
  if (!lexer)
    return BadArgs;
  if (!lexer->mf.data)
    return NullPointerField;
  if (!lexer->mf.size)
    return ZeroSize;
  Error err = OK;
  if ((err = daVerify(&lexer->tokens)))
    return err;
  return OK;
}

static const char* UNKNOWN_TOKEN_TYPE_STR = "UNKNOWN_TOKEN_TYPE";

const char* getTokenTypeStr(TokenType type) {
  if (type < 0 || type >= TOKEN_TYPES_SIZE)
    return UNKNOWN_TOKEN_TYPE_STR;

  return TOKEN_TYPES[type];
}

static bool isIn(char c, const char* str) {
  return strchr(str, c) != NULL;
}
