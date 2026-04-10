#include "diff/context.h"
#include "ds/tree/dump/dump.h"
#include "logger/logger.h"

int main(void) {
  loggerInit(".log/!latest.txt", DEBUG);
  logln(INFO, "Hello from main!");
  loglnTraced(WARN, "Hello from main, traced!");
  TreeNode* node = DIV_(COS_(MUL_(NUM_(123), NUM_(8))), NUM_(5));
  nodeFixParents(node);
  FILE* f = openHtmlLogFile(".log/");
  Context ctx = (Context){};
  contextInit(&ctx, 32);
  nodeDump(f, ctx.vars, node, "test");
  closeHtmlLogFile(f);
  nodeDestroy(node);
  contextDestroy(&ctx);
  loggerCloseFile();
  return 0;
}
