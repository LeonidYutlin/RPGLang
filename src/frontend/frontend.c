#include "io/io.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/lexer.h"
#include "frontend/preparser.h"
#include "frontend/parser.h"
#include <stdlib.h>
#include <string.h>

//TODO: do a tree traverse moving the exceptionCount upstream (up to statements)
static const char* DEFAULT_OUTPUT_FILEPATH = "ast.txt";
static void parseArgs(int* argc, char*** argv, 
               const char** input, const char** output);

int main(int argc, char* argv[]) {
  const char* input  = NULL;
  const char* output = NULL;
  parseArgs(&argc, &argv, &input, &output);

  int  exitValue = 0;
  bool loggerInited  = false;
  bool lexerInited   = false;
  //bool htmlLogInited = false;
  bool astInited     = false;
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

  //FILE* logFile = openHtmlLogFile("./.log/");
  //if (!logFile) {
  //  exitValue = FailFileOpen; 
  //  goto exit;
  //}
  //htmlLogInited = true;

  if ((err = lexerAnalyze(&lexer))) {
    logln(FATAL, "lexerAnalyze returned %s", parseError(err)->str);
    exitValue = err;
    goto exit;
  }

  //lexerPrintTokens(stdout, &lexer);
  preparse(&lexer.tokens);
  //printf("After Preparsing ------------\n");
  //lexerPrintTokens(stdout, &lexer);
  TreeNode* ast = parse(&lexer.tokens);
  if (!ast) {
    fprintf(stderr, "Failed to parse token stream\n");
    exitValue = Fail;
    goto exit;
  }
  astInited = true;
  //nodeDump(logFile, ast, "Parsed Tree");

  FILE* outFile = fopen(output, "w");
  if (!outFile) {
    fprintf(stderr, "Failed to open \"%s\" for write\n", output);
    exitValue = FailFileOpen;
    goto exit;
  }

  nodePrintPrefix(outFile, ast);

exit:
  if (loggerInited)
    loggerCloseFile();
  if (lexerInited)
    lexerDestroy(&lexer, false);
  //if (htmlLogInited)
  //  closeHtmlLogFile(logFile);
  if (astInited)
    nodeDestroy(ast);
  return exitValue;
}

// TODO: better usage desc
// TODO: better flag parsing?
static void parseArgs(int* argc, char*** argv, 
                      const char** input, const char** output) {
  if (*argc < 2) {
    fprintf(stderr, "Usage: %s <inputFilepath> -o <outputFilepath>\n", *argv[0]);
    exit(1);
  }
  popArg(argc, argv); // pop progs name, we wont need it from here
  const char* arg = NULL;
  while ((arg = popArg(argc, argv))) {
    if (*arg == '-') {
      arg++;
      if (strcmp(arg, "o") == 0) {
        *output = popArg(argc, argv);
        continue;
      }
      fprintf(stderr, "ERROR: Unknown flag\n");
    } else {
      if (*input) {
        fprintf(stderr, "ERROR: More than one input file is provided\n");
        exit(1);
      }
      *input = arg;
    }
  }
  if (!*output) {
    *output = DEFAULT_OUTPUT_FILEPATH;
    fprintf(stdout, 
            "WARN: no output filepath is provided. Proceeding with \"%s\"\n",
            *output);
  }
  return;
}
