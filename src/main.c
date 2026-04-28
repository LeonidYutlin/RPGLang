#include "ds/dump.h"
#include "ds/tree/node/type.h"
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

  //lexerPrintTokens(stdout, &lexer);
  TreeNode* ast = IF_(AND_(IDENT_("apple", 5), IDENT_("dog", 3)), NULL);
  TreeNode* ast2 = SEMIC_(IDENT_("foo", 3));
  TreeNode* ast3 = ELSE_(SEMIC_(IDENT_("bar", 5)));
  TreeNode* ast4 = SEMIC_(IDENT_("baz", 3));
  ast->right = ast2;
  ast2->right = ast3;
  ast3->right->right = ast4;
  nodeFixParents(ast);
  //parse(&lexer.tokens);
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
