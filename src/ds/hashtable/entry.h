#ifndef ENTRY_H
#define ENTRY_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  char* data;
  size_t size;
} StringView;

typedef struct {
  StringView key;
  uint64_t   hash;
  uint64_t   value; //TODO: change to tok type
} Entry;

#endif
