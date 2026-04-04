#include "diff/context.h"
#include "ds/tree/dump/dump.h"

int main(void) {
  TreeNode* node = MUL_(SIN_(ADD_(NUM_(100), NUM_(420))), NUM_(5));
  nodeFixParents(node);
  FILE* f = openHtmlLogFile(".log/");
  Context ctx = (Context){};
  contextInit(&ctx, 32);
  nodeDump(f, ctx.vars, node, "test");
  closeHtmlLogFile(f);
  nodeDestroy(node);
  contextDestroy(&ctx);
  return 0;
}
