#include "ds/tree/root.h"
#include <stdlib.h>

static Error countNodesCallback(TreeNode* node, uint level, void* data);

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
    RETURN_WITH_STATUS(BadArgs, NULL);

  TreeNode* node = root->rootNode;
  root->rootNode = NULL;
  free(root);
  return node;
}

Error rootDestroy(TreeRoot* root) {
  if (!root)
    return BadArgs;

  nodeDestroy(root->rootNode);
  free(root);

  return OK;
}

static Error countNodesCallback(_unused TreeNode* node, 
                                _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!node)
    return OK;
  size_t* nodeCount = (size_t*)data;
  (*nodeCount)++;
  return OK;
}
