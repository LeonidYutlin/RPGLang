#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include "error/error.h"

#define sizer(a) (sizeof((a)) / sizeof((a)[0]))

#if defined(__GNUC__) || defined(__clang__)
#define _unused __attribute__ ((unused))
#define _format(type, fmt_id, args_id) __attribute__ ((format (type, fmt_id, args_id)))
#else 
#define _unused 
#define _format
#endif

typedef struct {
  char* data;
  size_t size;
} StringView;

bool doubleEqual(double a, double b);

#define TIMESTAMP_BUF_SZ 32

/// prints up to n chars to dest, in a form of 
/// <prefix><timestamp><?-count><suffix>
///  | <?-count> - if count != 0, then "-%d"
///    else the count is not printed
///  | <timestamp> - see snTimestamp(...)
Error snTimestampedFilename(char* dest, size_t n, 
                            const char* prefix, 
                            const char* suffix, 
                            uint count);

/// prints up to n chars to dest, in a form of 
/// "%day-%month-%Year-%Hour:%Minute:%Second"
Error snTimestamp(char* dest, size_t n);

typedef struct MappedFile {
  char* data;
  size_t size;
} MappedFile; 


Error mappedFileInit(MappedFile* mappedFile, const char* filename);
Error mappedFileDestroy(MappedFile* mappedFile); 

char* popArg(int* argc, char*** argv);

void parseArgs(int* argc, char*** argv, 
               const char** input, const char** output);
#endif
