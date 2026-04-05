#include "error/error.h"
#include "utils/utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <float.h>

#define TIMESTAMP_BUF_SZ 32
static const double DOUBLE_COMPARISON_PRECISION = DBL_EPSILON;

bool doubleEqual(double a, double b) {
  return fabs(a - b) < DOUBLE_COMPARISON_PRECISION;
}

Error snTimestamp(char* dest, size_t n, 
                  const char* prefix, 
                  const char* suffix, 
                  uint count) {
  if (!prefix || !suffix)
    return InvalidParameters;

  char tsBuf[TIMESTAMP_BUF_SZ] = {};
  time_t timeAbs = time(NULL);
  struct tm* localTime = localtime(&timeAbs);
  if (!strftime(tsBuf, TIMESTAMP_BUF_SZ, 
                "%d-%m-%Y-%H:%M:%S", localTime))
    return LongFormat;

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

Error readBufferFromFile(FILE* file, char** bufferPtr, 
                         size_t* trueBufferSizePtr) {
  if (!file ||
      !bufferPtr ||
      !trueBufferSizePtr)
    return InvalidParameters;

  struct stat fileStats = {};
  fstat(fileno(file), &fileStats);
  size_t bufferSize = (size_t)fileStats.st_size;
  char* buffer = (char*)calloc(bufferSize + 1, sizeof(char)); // + 1 for '\0'
  if (!buffer)
    return FailMemoryAllocation;

  bufferSize = fread(buffer, sizeof(char), bufferSize, file);
  if (ferror(file))
    return FileError; 

  *trueBufferSizePtr = bufferSize;
  *bufferPtr         = buffer;
  return OK;
}
