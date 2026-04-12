#include "logger/logger.h"
#include "utils/utils.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
  loggerInit(".log/!latest.txt", DEBUG);

  int fd = open("src/main.c", O_RDONLY);
  if (fd < 0) {
    logln(FATAL, "Failed to open");
    return 1;
  }

  MappedFile mf = {};
  if (mappedFileInit(fd, &mf)) {
    logln(FATAL, "Failed to mappedFileInit");
    return 1;
  }

  logln(INFO, "Read file successfully!\n%s", mf.data);

  if (mappedFileDestroy(&mf)) {
    logln(FATAL, "Failed to mappedFileDestroy");
    return 1;
  }

  close(fd);

  loggerCloseFile();
  return 0;
}
