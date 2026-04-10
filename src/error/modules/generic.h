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
    "No errors occured. This always has the error code of 0")                      \
  X(Fail,                                                                          \
    GenericError,                                                                  \
    "Fail",                                                                        \
    "A function failed and it didn't specify why. "                                \
    "This always has the error code of 1")                                         \
  X(BadArgs,                                                                       \
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
  X(FileError,                                                                     \
    GenericError,                                                                  \
    "Std func has set ferror flag on the stream",                                  \
    "Std func has set ferror flag on the stream."                                  \
    "See perror() for more info")                                                  \
  X(NullPointerField,                                                              \
    GenericError,                                                                  \
    "There is a NULL field present in struct",                                     \
    "A crucial struct field is NULL, making the struct unusable in most cases")    \
  X(DenyReinit,                                                                    \
    GenericError,                                                                  \
    "An initialized struct was attempted to be reinitialized",                     \
    "An initialized struct was attempted to be reinitialized."                     \
    "If you really want to reinitialize it, destroy/deinitialize it first")        \
  X(LongFormat,                                                                    \
    GenericError,                                                                  \
    "Formatted string exceeds byte limit",                                         \
    "Formatting the string requires more bytes than the limit provided")

#endif
