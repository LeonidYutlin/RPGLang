#include "ds/dump.h"
#include "io/io.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/lexer.h"
#include "frontend/preparser.h"
#include "frontend/parser.h"
#include "frontend/symtab.h"
#include <string.h>

int main(int argc, char* argv[]) {
  const char* input  = NULL;
  const char* output = NULL;
  parseArgs(&argc, &argv, &input, &output);

  int  exitValue = 0;
  bool loggerInited  = false;
  bool lexerInited   = false;
  bool htmlLogInited = false;
  bool astInited     = false;
  bool symtabInited  = false;
  loggerInit(NULL, ERROR);
  loggerInited = true;

  Error err = OK;
  Lexer lexer = (Lexer){};
  if ((err = lexerInit(&lexer, input, 16))) {
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

  _unused bool valid = preparse(&lexer.tokens, &err);

  if (err) {
    fprintf(stderr, "Preparser Failed\n");
    exitValue = err;
    goto exit;
  }
#ifndef HARD_DIFFICULTY
  if (!valid) {
    fprintf(stderr, "Invalid class usage detected, no further compilation is done\n");
    exitValue = Fail;
    goto exit;
  }
#endif

  //printf("After Preparsing ------------\n");
  //lexerPrintTokens(stdout, &lexer);

  TreeNode* ast = parse(&lexer.tokens);
  if (!ast) {
    fprintf(stderr, "Failed to parse token stream\n");
    exitValue = Fail;
    goto exit;
  }
  astInited = true;
  nodeDump(logFile, ast, "Parsed Tree");

  FILE* outFile = fopen(output, "w");
  if (!outFile) {
    fprintf(stderr, "Failed to open \"%s\" for write\n", output);
    exitValue = FailFileOpen;
    goto exit;
  }

  static const size_t SYMTAB_BUCKET_SIZE = 17;
  static const size_t SYMTAB_LIST_CAPACITY = 4;
  static const hash_f SYMTAB_HASH_FUNC = hashRotate;
  TranslationUnit trUnit = (TranslationUnit){.ast = ast};
  if ((err = symtabInit(&trUnit, SYMTAB_BUCKET_SIZE, 
                        SYMTAB_LIST_CAPACITY, SYMTAB_HASH_FUNC))) {
    fprintf(stderr, "Failed to init symtab\n");
    exitValue = err;
    goto exit;
  }
  symtabInited = true;

  hashTableDump(logFile, &trUnit.symtab, "MANGLING");
  nodeDump(logFile, ast, "After Symtab Init");

  if (!symtabCheckCalls(&trUnit, &err)) {
    fprintf(stderr, "Invalid function call detected, no further compilation is done\n");
    exitValue = Fail;
    goto exit;
  }
  if (err) {
    fprintf(stderr, "Failed to check function calls\n");
    exitValue = err;
    goto exit;
  }

  nodePrintPrefix(outFile, ast);
  fclose(outFile);

exit:
  if (loggerInited)
    loggerCloseFile();
  if (lexerInited)
    lexerDestroy(&lexer, false);
  if (htmlLogInited)
    closeHtmlLogFile(logFile);
  if (astInited)
    nodeDestroy(ast);
  if (symtabInited)
    hashTableDestroy(&trUnit.symtab, false);
  return exitValue;
}
