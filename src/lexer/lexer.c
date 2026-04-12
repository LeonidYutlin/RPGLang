#include "lexer/lexer.h"
#include "utils/utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

Lexer* lexerAlloc(int fd, size_t initCap, Error* status) {
  if (fd < 0 || 
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

  if ((err = mappedFileInit(fd, &lexer->mf))) {
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
  
  const char* buf = lexer->mf.data;
  //Tokens* tokens = &lexer->tokens;
  //Token newToken = {};
  for (char c = buf[lexer->pos]; c != '\0'; c = buf[lexer->pos]) {
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
