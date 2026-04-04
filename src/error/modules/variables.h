#ifndef ERROR_MODULE_VAR_TABLE
#define ERROR_MODULE_VAR_TABLE

#define VARIABLES_ERROR_MODULE()                          \
  X(VariablesError,                                       \
    "Variables Errors",                                   \
    "Errors related to dyn array of variables - Variables")

#define VARIABLES_ERROR_LIST()                                                \
  X(BadCount,                                                                 \
    VariablesError,                                                           \
    "vars->count > vars->capacity",                                           \
    "The count field in Variables"                                            \
    "is greater than the capacity field. "                                    \
    "Potentially the count field has been corrupted or accidentally changed") \
  X(UnknownVariable,                                                          \
    VariablesError,                                                           \
    "Such variable is not present in Variables",                              \
    "There is no such variable in Variables. "                                \
    "if you wish to register it, call regVar()")                              \
  X(AttemptedReregistration,                                                  \
    VariablesError,                                                           \
    "Attempted to register an already registered variable",                   \
    "Attempted to register an already registered variable. "                  \
    "This error is soft and should mostly be ignored")

#endif
