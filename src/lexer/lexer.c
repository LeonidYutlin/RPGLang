#include "lexer/lexer.h"
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

#define EMIT(T, ...)                 \
  {                                  \
    newToken = (Token){              \
      __VA_ARGS__ __VA_OPT__(,)      \
      .type = T,                     \
      .line = lexer->line,           \
      .lineStart = lexer->lineStart, \
      .pos = lexer->pos,             \
    };                               \
    daAppend(tokens, &newToken);     \
  }

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
    switch (c) {
      case '(': EMIT(TOK_LPAR); lexer->pos++; continue;
      case ')': EMIT(TOK_RPAR); lexer->pos++; continue;
      default: break;
    }

    if (strchr("IVX", c) != NULL) {
      int num = 0;
      bool cont = true;
      int degree = 0;
      int newDegree = 0;
      int numBuf = 0;
      while (cont) {
        switch (c) {
          case 'I': newDegree = 1;  break;
          case 'V': newDegree = 5;  break;
          case 'X': newDegree = 10; break;
          default:  cont = false; num += numBuf; continue;
        }
        if (degree && 
            degree != newDegree) {
          if (degree < newDegree) {
            numBuf *= -1;
          }
          num += numBuf;
          numBuf = 0;
        }
        numBuf += newDegree;
        degree = newDegree;

        lexer->pos++;
        c = buf[lexer->pos];
      }
      EMIT(TOK_NUM_LIT, .value = num);
      continue;
    }
  }
  EMIT(TOK_EOF);
  return OK;
}

#undef EMIT

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
