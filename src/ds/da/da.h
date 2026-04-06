#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "error/error.h"
#include <stdbool.h>

typedef void (*free_f)(void* ptr);

typedef struct DynamicArray {
  void* items;
  size_t itemSize;
  size_t capacity;
  size_t count;
  free_f freeFunc;
} DynamicArray;

Error daInit(DynamicArray* dynamicArray, 
             size_t initialCapacity, size_t itemSize);
DynamicArray* daAlloc(size_t initialCapacity,
                      size_t itemSize, Error* status);
Error daAppend(DynamicArray* dynamicArray, void* elem);
Error daDestroy(DynamicArray* dynamicArray, bool isAlloced);
Error daVerify(DynamicArray* dynamicArray);

#endif
