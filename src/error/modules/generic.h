#ifndef ERROR_MODULE_GENERIC
#define ERROR_MODULE_GENERIC

#define GENERIC_ERROR_MODULE()   \
  X(GenericError,                \
    "Generic Errors",            \
    "Common errors across the whole project")

#define GENERIC_ERROR_LIST()                                                       \
  X(OK,                                                                            \
    GenericError,                                                                  \
    "Success",                                                                     \
    "No errors occured")                                                           \
  X(InvalidParameters,                                                             \
    GenericError,                                                                  \
    "Invalid parameters",                                                          \
    "Invalid parameters were provided for a function call")                        \
  X(FailMemoryAllocation,                                                          \
    GenericError,                                                                  \
    "Failed memory allocation",                                                    \
    "Failed to allocate memory (likely in m(c)alloc())")                           \
  X(FailMemoryReallocation,                                                        \
    GenericError,                                                                  \
    "Failed memory reallocation",                                                  \
    "Failed to reallocate memory (realloc()). "                                    \
    "Allocated memory is still stored where it was before attempted reallocation") \
  X(FailFileOpen,                                                                  \
    GenericError,                                                                  \
    "Failed to open file",                                                         \
    "Failed to open file. See perror() for more info")                             \
  X(EndOfFile,                                                                     \
    GenericError,                                                                  \
    "Std func returned EOF",                                                       \
    "Std func returned EOF. See perror() for more info")                           \
  X(NullPointerField,                                                              \
    GenericError,                                                                  \
    "There is a NULL field present in struct",                                     \
    "A crucial struct field is NULL, making the struct unusable in most cases")    \
  X(UnknownEnumItem,                                                               \
    GenericError,                                                                  \
    "Unknown enumeration item",                                                    \
    "An item with this enum value is not present in it's X'ed macro array. "       \
    "Check integrity of the X macros, otherwise this is a corrupted value")        \
  X(BadEnumItem,                                                                   \
    GenericError,                                                                  \
    "Unexpected enumeration item",                                                 \
    "An item with this enum value is wasn't expected. "                            \
    "It is likely that it is some kind of default value in an established enum")

#endif
