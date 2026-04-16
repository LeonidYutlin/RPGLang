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

  lexerPrintTokens(stdout, lexer); 

  lexerDestroy(lexer);
  close(fd);

  loggerCloseFile();
  return 0;
}
