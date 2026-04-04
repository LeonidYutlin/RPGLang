#include <stdlib.h>
#include "diff/io/io.h"
#include "diff/io/parse.h"
#include "diff/context.h"
#include "ds/tree/dump/dump.h"

int main(void) {
  TreeNode* node = nodeAlloc(NUM_UNIT_(69));
  FILE* f = openHtmlLogFile(".log/");
  Context ctx = (Context){};
  contextInit(&ctx, 32);
  nodeDump(f, ctx.vars, node, "test");
  closeHtmlLogFile(f);
  nodeDestroy(node);
  contextDestroy(&ctx);
  return 0;
}
