#ifndef ERROR_MODULE_TREE
#define ERROR_MODULE_TREE

#define TREE_ERROR_MODULE()                               \
  X(TreeError,                                            \
    "Tree/Node Errors",                                   \
    "Errors related to TreeRoot/TreeNode data structures")

#define TREE_ERROR_LIST()                                      \
  X(FailReadNode,                                              \
    TreeError,                                                 \
    "Failed to read node",                                     \
    "Failed to read node, due to incorrect syntax. "           \
    "See logs for more info")

#endif
