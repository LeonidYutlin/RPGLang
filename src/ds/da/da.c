#include "ds/da/da.h"
#include <stdlib.h>
#include <string.h>

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
      *status = value;                         \
  return returnValue;                          \
  }

DynamicArray* daAlloc(size_t initialCapacity, size_t itemSize, Error* status) {
  if (!initialCapacity || !itemSize)
    RETURN_WITH_STATUS(InvalidParameters, NULL);

  DynamicArray* da = (DynamicArray*)calloc(1, sizeof(DynamicArray));
  if (!da)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  void* items = calloc(initialCapacity, itemSize);
  if (!items) {
    free(da);
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);
  }

  da->items    = items;
  da->itemSize = itemSize;
  da->count    = 0;
  da->capacity = initialCapacity;

  return da;
}

#undef RETURN_WITH_STATUS

Error daDestroy(DynamicArray* da) {
  if (!da)
    return InvalidParameters;

  if (da->items) {
    char* endOfItems = (char*)da->items + da->capacity * da->itemSize;
    size_t itemSize = da->itemSize;
    free_f freeFunc = da->freeFunc;
    for (char* i = da->items; i < endOfItems; i += itemSize)
      freeFunc(i);
  }
  free(da->items);
  free(da);

  return OK;
}

Error daAppend(DynamicArray* da, void* elem) {
  if (!elem)
    return InvalidParameters;
  Error err = daVerify(da);
  if (err)
    return err;
 
  if (da->count == da->capacity) {
    size_t newCapacity = da->capacity * 2;
    void* temp = realloc(da->items, newCapacity * da->itemSize);
    if (!temp)
      return FailMemoryReallocation;
    da->capacity = newCapacity;
    da->items = temp;
  }

  memcpy((char*)da->items + da->itemSize * (da->count++), 
         elem, da->itemSize);

  return OK;
}

Error daVerify(DynamicArray* da) {
  if (!da)
    return InvalidParameters;
  if (!da->items)
    return NullPointerField;
  if (da->count > da->capacity)
    return BadCount;
  if (!da->capacity ||
      !da->itemSize)
    return ZeroSize;
  return OK;
}
