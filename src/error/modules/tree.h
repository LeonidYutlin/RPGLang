#ifndef ERROR_MODULE_TREE
#define ERROR_MODULE_TREE

#define TREE_ERROR_MODULE()                               \
  X(TreeError,                                            \
    "Tree/Node Errors",                                   \
    "Errors related to TreeRoot/TreeNode data structures")

#define TREE_ERROR_LIST()                                      \
  X(UninitializedTree,                                         \
    TreeError,                                                 \
    "Tree is uninitialized",                                   \
    "Tree is uninitialized, "                                  \
    "use nodeInit()/nodeAlloc() or tree- counterparts")        \
  X(DestroyedTree,                                             \
    TreeError,                                                 \
    "Tree is destroyed",                                       \
    "Tree has been destroyed before, "                         \
    "use initializers to reinit it")                           \
  X(AttemptedReinitialization,                                 \
    TreeError,                                                 \
    "Attempted to reinitialize a working tree",                \
    "Attempted to reinitialize a working tree, "               \
    "if you wish to reinit this tree, destroy it and init it") \
  X(FailReadNode,                                              \
    TreeError,                                                 \
    "Failed to read node",                                     \
    "Failed to read node, due to incorrect syntax. "           \
    "See logs for more info")

#endif
