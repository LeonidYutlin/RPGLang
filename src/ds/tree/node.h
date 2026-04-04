#ifndef NODE_H
#define NODE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include "ds/tree/nodetype.h"
#include "error/error.h"

typedef union NodeValue {
  OpType op;
  size_t var; //var_id actually
  double num;
} NodeValue;

typedef struct NodeUnit {
  NodeType type;
  NodeValue value;
} NodeUnit;

typedef struct TreeNode_ {
  NodeUnit  data;
  struct TreeNode_* parent;
  struct TreeNode_* left;
  struct TreeNode_* right;
} TreeNode;

Error nodeInit(TreeNode* node, NodeUnit data, TreeNode* parent,
               TreeNode* left, TreeNode* right);
TreeNode* nodeAlloc(NodeUnit data, TreeNode* parent,
                    TreeNode* left, TreeNode* right,
                    Error* status);

typedef Error (*callback_f)(TreeNode* node, void* data, uint level);

typedef struct NodeTraverseOpt {
  callback_f prefix;
  callback_f infix;
  callback_f postfix;
  void* prefixData;
  void* infixData ;
  void* postfixData;
  uint level;
} NodeTraverseOpt;

/// Universal Traverse - stops if any callback_f return non-zero
Error nodeTraverse(TreeNode* node, NodeTraverseOpt opt);

#define nodeTraverse(node, ...) \
  nodeTraverse(node, (NodeTraverseOpt){ __VA_ARGS__ })

Error countNodesCallback(TreeNode* node, void* data, uint level);
Error findVariableCallback(TreeNode* node, void* data, uint level);

//Takes parent, seaches for child as its child, replaces that child with newChild and frees child
//Note: if your newChild and child are in a relationship then you should make sure they aren't by
//breaking the bound before calling the function
Error nodeChangeChild(TreeNode* parent, TreeNode* child, TreeNode* newChild,
                      size_t* nodeCount);

///Note: doesnt copy the parent field but instead assigns newParent as copy's parent
TreeNode*  nodeCopy(TreeNode* srcNode, TreeNode* newParent, Error* status);
void nodeFixParents(TreeNode* node);
Error nodeOptimize(TreeNode** node);

Error  nodeDelete(TreeNode* node, bool isAlloced, size_t* nodeCount);
Error nodeDestroy(TreeNode* node, bool isAlloced, size_t* nodeCount);

#endif
