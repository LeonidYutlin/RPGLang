#include "lexer/lexer.h"
#include "utils/utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

static void skipWhitespace(Lexer* lexer);

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
      *status = value;                         \
  return returnValue;                          \
  }

Lexer* lexerAlloc(FILE* src, size_t initCap, Error* status) {
  if (!src || 
      !initCap)
    RETURN_WITH_STATUS(InvalidParameters, NULL);

  Lexer* lexer = (Lexer*)calloc(1, sizeof(Lexer));
  if (!lexer)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = daInit(&lexer->tokens, initCap, sizeof(Token)))) {
    free(lexer);
    RETURN_WITH_STATUS(err, NULL);
  }

  if ((err = readBufferFromFile(src, &lexer->buf, &lexer->bufSize))) {
    free(lexer);
    RETURN_WITH_STATUS(err, NULL);
  }

  lexer->pos        = 0;
  lexer->line       = 1;
  lexer->lineStart  = 1;
  return lexer;
}

#undef RETURN_WITH_STATUS

Error lexerAnalyze(Lexer* lexer) {
  Error err = OK;
  if ((err = lexerVerify(lexer)))
    return err;
  
  char* buf = lexer->buf;
  size_t curPos = lexer->pos;
  while (buf[curPos] != '\0') {
    skipWhitespace(lexer);
  }
  lexer->pos = curPos;
  return OK;
}

static void skipWhitespace(Lexer* lexer) {
  assert(lexer);

  char* buf = lexer->buf;
  size_t curPos = lexer->pos;
  for (char c = buf[curPos]; isspace(c); curPos++) {
    if (c == '\n') {
      lexer->line++;
      lexer->lineStart = curPos;
    }
  }
  lexer->pos = curPos;

  return;
}

Error lexerDestroy(Lexer* lexer) {
  if (!lexer)
    return InvalidParameters;
  
  daDestroy(&lexer->tokens, false);
  if (lexer->buf)
    free(lexer->buf);
    
  free(lexer);

  return OK;
}

Error lexerVerify(Lexer* lexer) {
  if (!lexer)
    return InvalidParameters;
  if (!lexer->buf)
    return NullPointerField;
  if (!lexer->bufSize)
    return ZeroSize;
  Error err = OK;
  if ((err = daVerify(&lexer->tokens)))
    return err;
  return OK;
}
