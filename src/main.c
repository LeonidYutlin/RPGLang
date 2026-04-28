#include "ds/dump.h"
#include "frontend/lexer.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/parser.h"
#include "frontend/preparser.h"
#include <fcntl.h>
#include <unistd.h>

//TODO: do a tree traverse moving the exceptionCount upstream (up to statements)

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
  loggerInit(NULL, ERROR);
  loggerInited = true;

  Error err = OK;
  Lexer lexer = (Lexer){};
  if ((err = lexerInit(&lexer, argv[1], 16))) {
    logln(FATAL, "lexerInit returned %s", parseError(err)->str);
    exitValue = err;
    goto exit;
  }
  lexerInited = true;

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
  preparse(&lexer.tokens);
  printf("After Preparsing ------------\n");
  lexerPrintTokens(stdout, &lexer);
  TreeNode* ast = parse(&lexer.tokens);
  if (!ast) {
    fprintf(stderr, "Failed to parse token stream\n");
    exitValue = Fail;
    goto exit;
  }
  astInited = true;
  nodeDump(logFile, ast, "Parsed Tree");

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
