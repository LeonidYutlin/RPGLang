#include "lexer/lexer.h"
#include "utils/utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

Lexer* lexerAlloc(FILE* src, size_t initCap, Error* status) {
  if (!src || 
      !initCap)
    RETURN_WITH_STATUS(BadArgs, NULL);

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

Error lexerAnalyze(Lexer* lexer) {
  Error err = OK;
  if ((err = lexerVerify(lexer)))
    return err;
  
  char* buf = lexer->buf;
  Tokens* tokens = &lexer->tokens;
  Token newToken = {};
  for (char c = buf[lexer->pos]; c != '\0';) {
    //skip whitespace
    if (isspace(c)) {
      if (c == '\n') {
        lexer->line++;
        lexer->lineStart = lexer->pos;
      }
      lexer->pos++;
      continue;
    }
  }
  //newToken = emitToken()
  //daAppend(tokens, )
  return OK;
}

//static void emitToken(Token* token, );

Error lexerDestroy(Lexer* lexer) {
  if (!lexer)
    return BadArgs;
  
  daDestroy(&lexer->tokens, false);
  if (lexer->buf)
    free(lexer->buf);
    
  free(lexer);

  return OK;
}

Error lexerVerify(Lexer* lexer) {
  if (!lexer)
    return BadArgs;
  if (!lexer->buf)
    return NullPointerField;
  if (!lexer->bufSize)
    return ZeroSize;
  Error err = OK;
  if ((err = daVerify(&lexer->tokens)))
    return err;
  return OK;
}
