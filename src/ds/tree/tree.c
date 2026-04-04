#include "ds/tree/tree.h"
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

TreeNode* detachRoot(TreeRoot* root, Error* status) {
  if (!root)
    RETURN_WITH_STATUS(InvalidParameters, NULL);

  TreeNode* node = root->rootNode;
  root->rootNode = NULL;
  treeDestroy(root, true);
  return node;
}

Error treeInit(TreeRoot* root, TreeNode* node, NodeUnit data,
                    TreeNode* left, TreeNode* right) {
  if (!root || !node)
    return InvalidParameters;
  if (root->rootNode)
    return AttemptedReinitialization;

  Error status = nodeInit(node, data, NULL, left, right);
  if (status)
    return status;
  root->rootNode = node;
  root->nodeCount = 1;

  return OK; //return treeVerify(root);
}

TreeRoot* treeAlloc(NodeUnit data,
                    TreeNode* left, TreeNode* right,
                    Error* status) {
  Error returnedStatus = OK;
  TreeNode* node = nodeAlloc(data, left, right, NULL, &returnedStatus);
  if (returnedStatus)
    RETURN_WITH_STATUS(returnedStatus, NULL);

  TreeRoot* root = attachRoot(node, &returnedStatus);
  if (returnedStatus)
    RETURN_WITH_STATUS(returnedStatus, NULL);

  return root;
}

Error treeDestroy(TreeRoot* root, bool isAlloced) {
  if (!root)
    return InvalidParameters;

  nodeDestroy(root->rootNode, isAlloced, NULL);

  if (isAlloced)
    free(root);

  return OK;
}

#undef RETURN_WITH_STATUS
