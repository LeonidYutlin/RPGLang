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

  /*
```
foo(bar; baz);
```   
  */
  //lexerPrintTokens(stdout, &lexer);
  TreeNode* ast = FUNC_CALL_(SIGNATURE_(DECL_(PRIM_(), IDENT_("func", 4)), SEMIC_(PARAM_(PRIM_(), IDENT_("param1", 6)))), SEMIC_(SEMIC_(IDENT_("param1", 6))));
  TreeNode* ast2 = SEMIC_(RETURN_(IDENT_("param2", 6)));
  TreeNode* ast3 = SEMIC_(PARAM_(FRAC_(), IDENT_("param2", 6)));
  ast->right->right = ast2;
  ast->left->right->right = ast3;
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
