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

  //loggerInit(".log/!latest.txt", DEBUG);
  loggerInit(NULL, DEBUG);

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    logln(FATAL, "Failed to open");
    return 1;
  }

  Error err = OK;
  Lexer* lexer = lexerAlloc(fd, 16, &err);
  if (err) {
    logln(FATAL, "lexerAlloc returned %s", parseError(err)->str);
    close(fd);
    loggerCloseFile(); 
    return 1;
  }

  FILE* logFile = openHtmlLogFile("./.log/");
  if (!logFile) {
    lexerDestroy(lexer);
    close(fd);
    loggerCloseFile(); 
    return 1;
  }
  //hashTableDump(logFile, &KEYWORD_HT, "test");

  if ((err = lexerAnalyze(lexer))) {
    logln(FATAL, "lexerAnalyze returned %s", parseError(err)->str);
    closeHtmlLogFile(logFile);
    lexerDestroy(lexer);
    close(fd);
    loggerCloseFile(); 
    return 1;
  }

  //lexerPrintTokens(stdout, lexer);
  TreeNode* ast = parse(&lexer->tokens);
  if (!ast) {
    fprintf(stderr, "Failed to parse\n");
    closeHtmlLogFile(logFile);
    lexerDestroy(lexer);
    close(fd);
    loggerCloseFile(); 
    return 1;
  }
  nodeDump(logFile, NULL, ast, "Parsed Tree");

  nodeDestroy(ast);
  closeHtmlLogFile(logFile);
  lexerDestroy(lexer);
  close(fd);
  loggerCloseFile();
  return 0;
}
