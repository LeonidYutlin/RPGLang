#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include "error/error.h"

#define sizer(a) (sizeof((a)) / sizeof((a)[0]))

#if defined(__GNUC__) || defined(__clang__)
#define unused __attribute__ ((unused))
#else 
#define unused 
#endif

bool doubleEqual(double a, double b);

/// prints up to n chars to dest, in a form of 
/// <prefix><timestamp><?-count><suffix>
///  | <?-count> - if count != 0, then "-%d"
///    else the count is not printed
///  | <timestamp> - "%day-%month-%Year-%Hour:%Minute:%Second"
Error snTimestamp(char* dest, size_t n, 
                  const char* prefix, 
                  const char* suffix, 
                  uint count);

Error readBufferFromFile(FILE* file,
                         char** buffer, size_t* bufferSize);

#endif
