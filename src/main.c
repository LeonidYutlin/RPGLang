#include "ds/dump.h"
#include "lexer/lexer.h"
#include "logger/logger.h"
#include "error/error.h"
#include "parser/parser.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
    return 1;
  }

  int  exitValue = 0;
  bool loggerInited  = false;
  bool lexerInited   = false;
  bool htmlLogInited = false;
  bool astInited     = false;
  loggerInit(NULL, DEBUG);
  loggerInited = true;

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    logln(FATAL, "Failed to open \"%s\"", argv[1]);
    exitValue = FailFileOpen;
    goto exit;
  }

  Error err = OK;
  Lexer lexer = (Lexer){};
  if ((err = lexerInit(&lexer, fd, 16))) {
    logln(FATAL, "lexerInit returned %s", parseError(err)->str);
    close(fd);
    exitValue = err;
    goto exit;
  }
  lexerInited = true;
  close(fd);

  FILE* logFile = openHtmlLogFile("./.log/");
  if (!logFile) {
    exitValue = FailFileOpen; 
    goto exit;
  }
  htmlLogInited = true;

  if ((err = lexerAnalyze(&lexer))) {
    logln(FATAL, "lexerAnalyze returned %s", parseError(err)->str);
    exitValue = err;
    goto exit;
  }

  lexerPrintTokens(stdout, &lexer);
  TreeNode* ast = parse(&lexer.tokens);
  if (!ast) {
    fprintf(stderr, "Failed to parse token stream\n");
    exitValue = Fail;
    goto exit;
  }
  astInited = true;
  nodeDump(logFile, NULL, ast, "Parsed Tree");

exit:
  if (loggerInited)
    loggerCloseFile();
  if (lexerInited)
    lexerDestroy(&lexer, false);
  if (htmlLogInited)
    closeHtmlLogFile(logFile);
  if (astInited)
    nodeDestroy(ast);
  return exitValue;
}
