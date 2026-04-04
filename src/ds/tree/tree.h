#ifndef TREE_H
#define TREE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "ds/tree/node.h"

typedef struct TreeRoot {
  size_t nodeCount;
  TreeNode* rootNode;
} TreeRoot;

TreeRoot* attachRoot(TreeNode* node, Error* status);
TreeNode* detachRoot(TreeRoot* root, Error* status);
Error treeInit(TreeRoot* root, TreeNode* node, NodeUnit data,
                    TreeNode* left, TreeNode* right);
TreeRoot* treeAlloc(NodeUnit data,
                    TreeNode* left, TreeNode* right,
                    Error* status);

#define treeTraverse(root, ...) \
  nodeTraverse((root)->rootNode, (NodeTraverseOpt){ __VA_ARGS__ })

Error treeDestroy(TreeRoot* tree, bool isAlloced);

#endif
