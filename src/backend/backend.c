#include "backend/codegen.h"
#include "ds/dump.h"
#include "ds/tree/node.h"
#include "ds/tree/type.h"
#include "io/io.h"
#include "utils/utils.h"

static Error mergeExceptionsCallback(TreeNode* node, uint level, void* data);

int main(int argc, char* argv[]) {
  const char* input  = NULL;
  const char* output = NULL;
  parseArgs(&argc, &argv, &input, &output);

  int  exitValue = 0;
  bool loggerInited  = false;
  bool htmlLogInited = false;
  bool mapFileInited = false;
  bool astInited     = false;
  loggerInit(NULL, ERROR);
  loggerInited = true;

  MappedFile mf = {};
  if ((exitValue = mappedFileInit(&mf, input))) {
    fprintf(stderr, "mappedFileInit returned %s\n", parseError(exitValue)->str);
    goto exit;
  }
  mapFileInited = true;

  TreeNode* ast = nodeRead(&mf, &exitValue);
  if (exitValue) {
    fprintf(stderr, "nodeRead returned %s\n", parseError(exitValue)->str);
    goto exit;
  }
  astInited = true;

  FILE* logFile = openHtmlLogFile("./.log/");
  if (!logFile) {
   exitValue = FailFileOpen; 
   goto exit;
  }
  htmlLogInited = true;

  nodeDump(logFile, ast, "<b2>hello</b2>");
  // TODO: factor this out
  uint64_t* excPtr = NULL;
  nodeTraverse(ast, 
               .prefix = mergeExceptionsCallback, 
               .prefixData = &excPtr);
  nodeDump(logFile, ast, "<b2>merged</b2>");

  FILE* outFile = fopen(output, "w");
  if (!outFile) {
   fprintf(stderr, "Failed to open \"%s\" for write\n", output);
   exitValue = FailFileOpen;
   goto exit;
  }
  codegen(outFile, ast);
  
  fclose(outFile);

exit:
  if (loggerInited)
    loggerCloseFile();
  if (htmlLogInited)
    closeHtmlLogFile(logFile);
  if (astInited)
    nodeDestroy(ast);
  if (mapFileInited)
    mappedFileDestroy(&mf);
  return exitValue;
}

static Error mergeExceptionsCallback(TreeNode* node, 
                                     _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!node)
    return OK;

  uint64_t** exc = (uint64_t**)data;
  if (OF_CTRL(node, CTRL_SEMIC) ||
      OF_CTRL(node, CTRL_IF)) {
    *exc = &node->data.exceptionCount;
    return OK;
  }

  if (!*exc)
    return OK;

  **exc += node->data.exceptionCount;
  node->data.exceptionCount = 0;
  return OK;
}
