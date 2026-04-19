#include "list.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

const ListUnit LIST_UNIT_CANARY = (ListUnit){
  .key = {"Wal@pa1@", 8},
  .hash = 0xBABABABA,
  .value = 0xABDEFFFF,
};
const ListUnit LIST_UNIT_ZERO = (ListUnit){0};

static const size_t REALLOC_MULT = 2;

static ListIndex  linkNewUnit(List* lst, ListIndex prev, ListIndex next, ListUnit value);
static Error reallocateList(List* lst, size_t newCapacity);

Error listInit(List* lst, size_t initialCapacity, bool isDoubleLinked) {
  if (!lst || !initialCapacity)
    return BadArgs;
  if (lst->initialized)
    return DenyReinit;
  Error err = listVerify(lst);
  if (err != OK &&
      err != Uninitialized)
    return err;

  /// +1 for the fictional 0th element
  size_t actualCapacity = initialCapacity + 1;
  ListUnit*  tempDataPtr = (ListUnit*)calloc(actualCapacity, sizeof(ListUnit));
  ListIndex* tempNextPtr = (ListIndex*)calloc(actualCapacity, sizeof(ListIndex));
  ListIndex* tempPrevPtr = isDoubleLinked
                           ? (ListIndex*)calloc(actualCapacity, sizeof(ListIndex))
                           : NULL;
  if (!tempDataPtr || !tempNextPtr || 
      (isDoubleLinked && !tempPrevPtr)) {
    free(tempDataPtr); free(tempNextPtr); free(tempPrevPtr);
    return FailMemoryAllocation;
  }

  *lst = (List){
    .data = tempDataPtr,
    .next = tempNextPtr,
    .prev = tempPrevPtr,
    .isDoubleLinked = isDoubleLinked,
    .free = 1,
    .freeTail = actualCapacity - 1,
    .status = OK,
    .capacity = actualCapacity,
    .initialized = true, 
  };

  lst->data[0] = LIST_UNIT_CANARY;
  for (ListIndex i = 2; i < actualCapacity; i++)
    lst->next[i - 1] = i;

  return OK;
}

List* listAlloc(size_t initialCapacity, bool isDoubleLinked, Error* status) {
  if (initialCapacity == 0)
    RETURN_WITH_STATUS(BadArgs, NULL);

  List* lst = (List*)calloc(1, sizeof(List));
  if (!lst)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = listInit(lst, initialCapacity, isDoubleLinked))) {
    free(lst);
    RETURN_WITH_STATUS(err, NULL);
  }

  return lst;
}

#define IS_INVALID_INDEX(index) \
  (index > lst->capacity ||     \
   (lst->isDoubleLinked && index != lst->next[0] && lst->prev[index] == 0))

#define VERIFY_LIST()          \
  Error err = listVerify(lst); \
  if (err)                     \
    RETURN_WITH_STATUS(err, 0);

ListIndex listAddAfter(List* lst, ListIndex index, ListUnit value, Error* status) {
  VERIFY_LIST();
  if (IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  if (lst->free == 0 && 
      (err = reallocateList(lst, lst->capacity * REALLOC_MULT - 1)))
    RETURN_WITH_STATUS(err, 0);

  return linkNewUnit(lst, index, lst->next[index], value);
}

ListIndex listAddBefore(List* lst, ListIndex index, ListUnit value, Error* status) {
  VERIFY_LIST();
  if (!lst->isDoubleLinked)
    RETURN_WITH_STATUS(BadArgs, 0);
  if (IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  if (lst->free == 0 && 
      (err = reallocateList(lst, lst->capacity * REALLOC_MULT - 1)))
    RETURN_WITH_STATUS(err, 0);

  return linkNewUnit(lst, lst->prev[index], index, value);
}

ListIndex listAddAfterTail(List* lst, ListUnit value, Error* status) {
  return listAddAfter(lst, lst->prev[0], value, status);
}

ListIndex listAddAfterHead(List* lst, ListUnit value, Error* status) {
  return listAddAfter(lst, lst->next[0], value, status);
}

ListIndex listAddBeforeTail(List* lst, ListUnit value, Error* status) {
  return listAddBefore(lst, lst->prev[0], value, status);
}

ListIndex listAddBeforeHead(List* lst, ListUnit value, Error* status) {
  return listAddBefore(lst, lst->next[0], value, status);
}

Error listDelete(List* lst, ListIndex index) {
  Error err = listVerify(lst);
  if (err)
    return err;
  if (!lst->isDoubleLinked)
    return BadArgs;
  if (IS_INVALID_INDEX(index) ||
      index == 0)
    return BadArgs;

  lst->next[lst->prev[index]] = lst->next[index];
  lst->prev[lst->next[index]] = lst->prev[index];
  lst->data[index] = LIST_UNIT_ZERO;
  ListIndex oldFree = lst->free;
  lst->free = index;
  if (!lst->freeTail)
    lst->freeTail = index;
  lst->next[index] = oldFree;
  lst->prev[index] = 0;

  return OK;
}

Error listDeleteHead(List* lst) {
  return listDelete(lst, lst->next[0]);
}

Error listDeleteTail(List* lst) {
  return listDelete(lst, lst->prev[0]);
}

Error listDestroy(List* lst, bool isAlloced) {
  if (!lst)
    return BadArgs;

  free(lst->data);
  free(lst->next);
  free(lst->prev);

  *lst = (List){};

  if (isAlloced)
    free(lst);

  return OK;
}

ListIndex listGetHead(List* lst, Error* status) {
  return listGetNext(lst, 0, status);
}

ListIndex listGetTail(List* lst, Error* status) {
  return listGetPrev(lst, 0, status);
}

size_t listGetCapacity(List* lst, Error* status) {
  VERIFY_LIST();

  return lst->capacity - 1;
}

ListIndex listGetPrev(List* lst, ListIndex index, Error* status) {
  VERIFY_LIST();
  if (!lst->isDoubleLinked)
    RETURN_WITH_STATUS(BadArgs, 0);
  if (IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  return lst->prev[index];
}

ListIndex listGetNext(List* lst, ListIndex index, Error* status) {
  VERIFY_LIST();
  if (IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  return lst->next[index];
}

ListUnit listGetValue(List* lst, ListIndex index, Error* status) {
  Error err = listVerify(lst);
  if (err)
    RETURN_WITH_STATUS(err, LIST_UNIT_ZERO);
  if (IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, LIST_UNIT_ZERO);

  return lst->data[index];
}

#undef VERIFY_LIST

Error listSetValue(List* lst, ListIndex index, ListUnit value) {
  Error err = listVerify(lst);
  if (err)
    return err;
  if (IS_INVALID_INDEX(index))
    return BadArgs;

  lst->data[index] = value;

  return OK;
}

#define CHECK(condition, newStatus)   \
  {                                   \
    if (condition)                    \
    return (lst->status = newStatus); \
  }

Error listVerify(List* lst) {
  if (!lst)
    return BadArgs;
  if (!lst->initialized)
    return Uninitialized;
  if (!lst->capacity)
    return ZeroSize;
  CHECK(!lst->data || !lst->next ||
        (lst->isDoubleLinked && !lst->prev), 
        NullPointerField);
  CHECK(lst->prev[0]  > lst->capacity ||
        lst->next[0]  > lst->capacity ||
        lst->free     > lst->capacity ||
        lst->freeTail > lst->capacity, 
        BadIndex);
  CHECK(LIST_UNIT_COMPARISON(lst->data[0], LIST_UNIT_CANARY) != 0, 
        CorruptedCanary);
  return (lst->status = OK);
}

#undef CHECK

#define RETURN_IF_LOOPED(index)               \
{                                             \
  if (beenVisited[cur]) {                     \
    return (lst->status = LoopedConnections); \
  }                                           \
  beenVisited[cur] = true;                    \
  totalVisited++;                             \
}

Error listLoopCheck(List* lst) {
  Error err = listVerify(lst);
  if (err)
    return err;

  bool* beenVisited = (bool*)alloca(lst->capacity * sizeof(bool));
  memset(beenVisited, 0, lst->capacity * sizeof(bool));
  uint64_t totalVisited = 0;
  if (lst->next[0]) {
    for (ListIndex cur = lst->next[0];
         lst->next[cur] < lst->capacity;
         cur = lst->next[cur]) {
      RETURN_IF_LOOPED(cur);
      if (!lst->next[cur])
        break;
    }
  }
  if (lst->free) {
    for (ListIndex cur = lst->free;
         lst->next[cur] < lst->capacity;
         cur = lst->next[cur]) {
      RETURN_IF_LOOPED(cur);
      if (!lst->next[cur])
        break;
    }
  }

  return totalVisited == lst->capacity - 1
         ? OK
         : (lst->status = DanglingUnit);
}

#undef RETURN_IF_LOOPED

Error listLinearize(List* lst) {
  Error err = listVerify(lst);
  if (err)
    return err;

  List temp = (List){};
  if ((err = listInit(&temp, listGetCapacity(lst, NULL), lst->isDoubleLinked))) {
    listDestroy(&temp, false);
    return err;
  }

  for (ListIndex cur = lst->next[0];
       lst->next[cur] < lst->capacity;
       cur = lst->next[cur]) {
    listAddAfterTail(&temp, lst->data[cur], &err);
    if (err) {
      listDestroy(&temp, false);
      return err;
    }
    if (!lst->next[cur])
      break;
  }

  listDestroy(lst, false);
  *lst = temp;

  return OK;
}

static Error reallocateList(List* lst, size_t newCapacity) {
  assert(lst);
  assert(newCapacity > lst->capacity);

  ListUnit*  tempDataPtr = (ListUnit*)realloc(lst->data, newCapacity * sizeof(ListUnit));
  ListIndex* tempNextPtr = (ListIndex*)realloc(lst->next, newCapacity * sizeof(ListIndex));
  ListIndex* tempPrevPtr = lst->isDoubleLinked
                           ? (ListIndex*)realloc(lst->prev, newCapacity * sizeof(ListIndex))
                           : NULL;
  if (!tempDataPtr || !tempNextPtr || 
      (lst->isDoubleLinked && !tempPrevPtr))
    return FailMemoryReallocation;

  lst->data = tempDataPtr;
  lst->next = tempNextPtr;
  lst->prev = tempPrevPtr;

  if (lst->free == 0) { // if no free units left
    lst->free = lst->capacity;
  } else {
    lst->next[lst->freeTail] = lst->capacity;
  }
  lst->freeTail = newCapacity - 1;

  // initialize new free elements
  for (ListIndex i = lst->capacity; i < newCapacity; i++) {
    lst->data[i] = LIST_UNIT_ZERO;
    if (lst->isDoubleLinked)
      lst->prev[i] = 0;
    if (i != lst->capacity)
      lst->next[i - 1] = i;
  }
  lst->next[newCapacity - 1] = 0;

  lst->capacity = newCapacity;

  return OK;
}

static ListIndex linkNewUnit(List* lst, ListIndex prev, ListIndex next, ListUnit value) {
  assert(lst);

  ListIndex center = lst->free;

  lst->free = lst->next[center];
  if (center == lst->freeTail)
    lst->freeTail = 0;

  lst->data[center] = value;
  lst->next[center] = next;
  lst->next[prev]   = center;
  if (lst->isDoubleLinked) {
    lst->prev[center] = prev;
    lst->prev[next]   = center;
  }

  return center;
}

#undef IS_INVALID_INDEX
