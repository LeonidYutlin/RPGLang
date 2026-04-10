#ifndef LOGGER_H
#define LOGGER_H

#include "error/error.h"
#include <stddef.h>
#include <stdio.h>

#define LOG_LEVEL_LIST() \
  X(DEBUG)               \
  X(INFO)                \
  X(WARN)                \
  X(ERROR)               \
  X(FATAL)

typedef enum {
  #define X(enm) enm,
  LOG_LEVEL_LIST()
  #undef X
} LogLevel;

extern const size_t LOG_LEVELS_SIZE; 

typedef struct Logger {
  FILE* sink;
  LogLevel level;
} Logger;

/// filename can be NULL, this way logger sets the output to stderr
Error loggerInit(const char* filename, LogLevel level);
void  loggerCloseFile();

#define logln(level, fmt, ...) \
  logln_(level, fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#define loglnTraced(level, fmt, ...) \
  logln(level, "[%s:%d in %s] " fmt, \
        __FILE__, __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__)

Error logln_(LogLevel level, const char* fmt, ...);

#endif
