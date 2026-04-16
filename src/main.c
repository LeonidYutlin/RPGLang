#include "lexer/lexer.h"
#include "logger/logger.h"
#include "error/error.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
    return 1;
  }

  loggerInit(".log/!latest.txt", DEBUG);

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    logln(FATAL, "Failed to open");
    return 1;
  }

  Error err = OK;
  Lexer* lexer = lexerAlloc(fd, 16, &err);
  if (err) {
    logln(FATAL, "lexerAlloc returned %s", parseError(err)->str);
    return 1;
  }

  if ((err = lexerAnalyze(lexer))) {
    logln(FATAL, "lexerAnalyze returned %s", parseError(err)->str);
    return 1;
  }

  for (size_t i = 0; i < lexer->tokens.count; i++) {
    Token* t = (Token*)daGet(&lexer->tokens, i);
    switch (t->type) {
      case TOK_NUM_LIT:
        printf("%s(%lu)(%.*s)\n", 
               getTokenTypeStr(t->type), t->value, 
               (int)t->len, lexer->mf.data + t->pos);
        break;
      case TOK_IDENTIFIER:
        printf("%s(%.*s)\n", 
               getTokenTypeStr(t->type), 
               (int)t->len, lexer->mf.data + t->pos);
        break;
      default:
        printf("%s\n", getTokenTypeStr(t->type));
    }
  }

  lexerDestroy(lexer);
  close(fd);

  loggerCloseFile();
  return 0;
}
