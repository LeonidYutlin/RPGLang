#ifndef ENTRY_H
#define ENTRY_H

#include "lexer/lexer.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  StringView key;
  uint64_t   hash;
  TokenType  value;
} Entry;

#endif
