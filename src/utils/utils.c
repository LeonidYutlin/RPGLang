#include "error/error.h"
#include "utils/utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <float.h>

static const double DOUBLE_COMPARISON_PRECISION = DBL_EPSILON;

bool doubleEqual(double a, double b) {
  return fabs(a - b) < DOUBLE_COMPARISON_PRECISION;
}

Error snTimestampedFilename(char* dest, size_t n, 
                            const char* prefix, 
                            const char* suffix, 
                            uint count) {
  if (!prefix || !suffix)
    return BadArgs;

  char tsBuf[TIMESTAMP_BUF_SZ] = {};
  Error err = OK;
  if ((err = snTimestamp(tsBuf, TIMESTAMP_BUF_SZ)))
    return err;

  if (count) {
    if (snprintf(dest, n, 
                 "%s%s-%u%s", 
                 prefix, tsBuf, 
                 count, suffix) <= 0)
      return LongFormat;
  } else {
    if (snprintf(dest, n, 
                 "%s%s%s", 
                 prefix, tsBuf,
                 suffix) <= 0)
      return LongFormat;
  }

  return OK;
}

Error snTimestamp(char* dest, size_t n) {
  time_t timeAbs = time(NULL);
  struct tm* localTime = localtime(&timeAbs);
  return strftime(dest, n, "%d-%m-%Y-%H:%M:%S", localTime)
         ? OK
         : LongFormat;
}

Error mappedFileInit(int fd, MappedFile* mappedFile) {
  if (fd < 0 ||
      !mappedFile)
    return BadArgs;
  if (mappedFile->data)
    return DenyReinit;

  struct stat fileStats = {};
  fstat(fd, &fileStats);
  size_t fileSize = (size_t)fileStats.st_size;
  char *buffer = mmap(NULL, fileSize,
                      PROT_READ, MAP_PRIVATE,
                      fd, 0);
  if (buffer == MAP_FAILED)
    return FailMemoryMapping;

  mappedFile->size = fileSize;
  mappedFile->data = buffer;
  return OK;
}

Error mappedFileDestroy(MappedFile* mappedFile) {
  if (!mappedFile)
    return BadArgs;
  if (!mappedFile->data)
    return NullPointerField;

  if (munmap(mappedFile->data, mappedFile->size) < 0)
    return FailMemoryUnmapping;

  mappedFile->data = NULL;
  mappedFile->size = 0;
  return OK;
}

uint64_t hash(StringView strView) {
  uint64_t hash = 5381;

  for (size_t i = 0; i < strView.size; i++)
    hash = ((hash << 5) + hash) + (uint64_t)strView.data[i]; /* hash * 33 + c */

  //logln(INFO, "%.*s is %lu", strView.size, strView.data, hash);
  return hash;
}
