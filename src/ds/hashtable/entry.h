#ifndef ENTRY_H
#define ENTRY_H

#include "lexer/lexer.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  char* data;
  size_t size;
} StringView;

typedef struct {
  StringView key;
  uint64_t   hash;
  TokenType  value;
} Entry;

#endif
