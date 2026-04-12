#include "diff/context.h"
#include "ds/tree/dump/dump.h"
#include "logger/logger.h"
#include "utils/utils.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  loggerInit(".log/!latest.txt", DEBUG);

  FILE* f = fopen("src/main.c", "r"); 

  char* buf = NULL;
  size_t bufSize = 0;
  if (readBufferFromFile(f, &buf, &bufSize)) {
    logln(FATAL, "Failed to readBufferFromFile");
    return 1;
  }

  logln(INFO, "Read file successfully!\n%s", buf);
  free(buf);

  struct stat stats;
  Error err = fstat(fileno(f), &stats);
  if (err < 0) {
    logln(FATAL, "Failed to stat");
    return 1;
  }

  char *ptr = mmap(NULL, (size_t)stats.st_size,
                   PROT_READ, MAP_PRIVATE,
                   fileno(f), 0);
  if (ptr == MAP_FAILED) {
    logln(FATAL, "mmap failed");
    return 1;
  }

  logln(INFO, "Read file successfully!\n%s", ptr);

  err = munmap(ptr, (size_t)stats.st_size);
  if (err < 0) {
    logln(FATAL, "Failed to munmap");
    return 1;
  }

  fclose(f);

  //logln(INFO, "Hello from main!");
  //loglnTraced(WARN, "Hello from main, traced!");
  //TreeNode* node = DIV_(COS_(MUL_(NUM_(123), NUM_(8))), NUM_(5));
  //nodeFixParents(node);
  //FILE* f = openHtmlLogFile(".log/");
  //Context ctx = (Context){};
  //contextInit(&ctx, 32);
  //nodeDump(f, ctx.vars, node, "test");
  //closeHtmlLogFile(f);
  //nodeDestroy(node);
  //contextDestroy(&ctx);

  loggerCloseFile();
  return 0;
}
