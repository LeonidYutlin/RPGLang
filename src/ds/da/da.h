#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "error/error.h"
#include <assert.h>
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
             size_t initialCapacity, 
             size_t itemSize, free_f freeFunc);
DynamicArray* daAlloc(size_t initialCapacity, size_t itemSize,
                      free_f freeFunc, Error* status);
Error daAppend(DynamicArray* dynamicArray, void* elem);
Error daDestroy(DynamicArray* dynamicArray, bool isAlloced);
Error daVerify(DynamicArray* dynamicArray);

#define daGet(da, index) ((char*)((da)->items) + index * (da)->itemSize)


#endif
