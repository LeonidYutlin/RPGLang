#include "lexer/lexer.h"
#include "utils/utils.h"
#include <stdlib.h>

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
  Tokens* tokens = daAlloc(initCap, sizeof(Token), &err);
  if (err) {
    free(lexer);
    RETURN_WITH_STATUS(err, NULL);
  }

  char* buf = NULL;
  size_t bufSize = 0;
  if ((err = readBufferFromFile(src, &buf, &bufSize))) {
    free(tokens);
    free(lexer);
    RETURN_WITH_STATUS(err, NULL);
  }

  lexer->pos     = 0;
  lexer->line    = 1;
  lexer->offset  = 1;
  lexer->tokens  = tokens;
  lexer->buf     = buf;
  lexer->bufSize = bufSize;
  return lexer;
}

#undef RETURN_WITH_STATUS

//Error  lexerAnalyze(Lexer* lexer) {}

Error  lexerDestroy(Lexer* lexer) {
  if (!lexer)
    return InvalidParameters;
  
  if (lexer->tokens)
    daDestroy(lexer->tokens);
  if (lexer->buf)
    free(lexer->buf);
    
  free(lexer);

  return OK;
}

Error lexerVerify(Lexer* lexer) {
  if (!lexer)
    return InvalidParameters;
  if (!lexer->tokens ||
      !lexer->buf)
    return NullPointerField;
  if (!lexer->bufSize)
    return ZeroSize;
  Error err = OK;
  if ((err = daVerify(lexer->tokens)))
    return err;
  return OK;
}
