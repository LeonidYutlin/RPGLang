#include "ds/tree/root.h"
#include "ds/tree/node/callback.h"
#include <stdlib.h>

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
      *status = value;                         \
  return returnValue;                          \
  }

TreeRoot* attachRoot(TreeNode* node, Error* status) {
  TreeRoot* root = (TreeRoot*)calloc(1, sizeof(TreeRoot));
  if (!root)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  root->rootNode = node;
  nodeTraverse(node, .infix = countNodesCallback, .infixData = &root->nodeCount);
  return root;
}

TreeRoot* attachRootC(TreeNode* node, size_t nodeCount, Error* status) {
  TreeRoot* root = (TreeRoot*)calloc(1, sizeof(TreeRoot));
  if (!root)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  root->rootNode = node;
  root->nodeCount = nodeCount;
  return root;
}

TreeNode* detachRoot(TreeRoot* root, Error* status) {
  if (!root)
    RETURN_WITH_STATUS(InvalidParameters, NULL);

  TreeNode* node = root->rootNode;
  root->rootNode = NULL;
  free(root);
  return node;
}

Error rootDestroy(TreeRoot* root) {
  if (!root)
    return InvalidParameters;

  nodeDestroy(root->rootNode);
  free(root);

  return OK;
}

#undef RETURN_WITH_STATUS
