#include "misc/util.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <float.h>

static const size_t TIMESTAMP_LEN = 128;
static const double DOUBLE_COMPARISON_PRECISION = DBL_EPSILON;

bool doubleEqual(double a, double b) {
  return fabs(a - b) < DOUBLE_COMPARISON_PRECISION;
}

#define DEFER() { \
  free(str);      \
  return NULL;    \
}
#define REMAINING_LEN (TIMESTAMP_LEN - (size_t)(target - str))
char* getTimestampedString(const char* prefix, const char* suffix, uint count) {
  time_t timeAbs = time(NULL);
  struct tm* localTime  = localtime(&timeAbs);
  char* str = (char*)calloc(TIMESTAMP_LEN, sizeof(char));
  if (!str)
    DEFER();

  char* target = str;
  strncat(target, prefix, REMAINING_LEN - 1);
  target += strlen(prefix);
  size_t n = strftime(target, REMAINING_LEN, "%d-%m-%Y-%H:%M:%S", localTime);
  if (!n) 
    DEFER();
  target += n;
  if (count) {
    if (snprintf(target, REMAINING_LEN, "-%u%s", count, suffix) <= 0)
      DEFER();
  } else {
    if (snprintf(target, REMAINING_LEN, "%s", suffix) <= 0)
      DEFER();
  }
  return str;
}
#undef DEFER
#undef REMAINING_LEN

Error readBufferFromFile(FILE* file,
                         char** bufferPtr, size_t* trueBufferSizePtr) {
  if (!file ||
      !bufferPtr ||
      !trueBufferSizePtr)
    return InvalidParameters;

  struct stat fileStats = {0};
  fstat(file->_fileno, &fileStats);
  size_t bufferSize = (size_t) fileStats.st_size;
  char* buffer = (char*) calloc(bufferSize + 1, sizeof(char)); // + 1 for NULL terminating character
  if (!buffer)
    return FailMemoryAllocation;

  bufferSize = fread(buffer, sizeof(char), bufferSize, file);

  *trueBufferSizePtr = bufferSize;
  *bufferPtr = buffer;

  return OK;
}
