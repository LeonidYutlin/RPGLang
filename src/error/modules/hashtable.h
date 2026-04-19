#ifndef ERROR_MODULE_HASH_TABLE
#define ERROR_MODULE_HASH_TABLE

#define HASH_TABLE_ERROR_MODULE()          \
  X(HashTableError,                        \
    "DynamicArray Errors",                 \
    "Errors related to dynamic arrays, "   \
    "whether DynamicArray struct itself, " \
    "or any other dynamic arrays on the heap")

#define HASH_TABLE_ERROR_LIST()                                               \
  X(NotFound,                                                                 \
    HashTableError,                                                           \
    "Element couldn't be found",                                              \
    "An element with such key doesn't exist in HashTable")

#endif
