#include "ds/dump.h"
#include "io/io.h"
#include "middleend/optimization.h"
#include "utils/utils.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  const char* input  = NULL;
  const char* output = NULL;
  parseArgs(&argc, &argv, &input, &output);

  int  exitValue = 0;
  bool loggerInited  = false;
  //bool htmlLogInited = false;
  bool mapFileInited = false;
  bool trUnitInited  = false;
  loggerInit(NULL, ERROR);
  loggerInited = true;

  MappedFile mf = {};
  if ((exitValue = mappedFileInit(&mf, input))) {
    fprintf(stderr, "mappedFileInit returned %s\n", parseError(exitValue)->str);
    goto exit;
  }
  mapFileInited = true;

  TranslationUnit trUnit = (TranslationUnit){};
  if ((exitValue = translationUnitRead(&mf, &trUnit))) {
    fprintf(stderr, "translationUnitRead returned %s\n", parseError(exitValue)->str);
    goto exit;
  }
  trUnitInited = true;

  // FILE* logFile = openHtmlLogFile("./.log/");
  // if (!logFile) {
  //   exitValue = FailFileOpen; 
  //   goto exit;
  // }
  //htmlLogInited = true;


  //hashTableDump(logFile, &trUnit.symtab, "MANGLING");
  //nodeDump(logFile, trUnit.ast, "<b2>hello</b2>");
  nodeOptimize(&trUnit.ast);
  //nodeDump(logFile, trUnit.ast, "<b2>hello again</b2>");

  FILE* outFile = fopen(output, "w");
  if (!outFile) {
    fprintf(stderr, "Failed to open \"%s\" for write\n", output);
    exitValue = FailFileOpen;
    goto exit;
  }
  translationUnitPrint(outFile, &trUnit);
  fclose(outFile);

exit:
  if (loggerInited)
    loggerCloseFile();
  //if (htmlLogInited)
    //closeHtmlLogFile(logFile);
  if (trUnitInited) {
    hashTableDestroy(&trUnit.symtab, false);
    nodeDestroy(trUnit.ast);
  }
  if (mapFileInited)
    mappedFileDestroy(&mf);
  return exitValue;
}
