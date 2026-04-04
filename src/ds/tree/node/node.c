#include "ds/tree/tree.h"
#include "misc/utils.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#undef nodeTraverse

static double nodeOptimizeConstants(TreeNode* node, size_t* nodeCount, Error* status);
static Error nodeOptimizeNeutral(TreeNode** node, size_t* nodeCount);

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
    *status = value;                           \
  return returnValue;                          \
  }

Error nodeTraverse(TreeNode* node, NodeTraverseOpt opt) {
	if (!node)
    return OK;

  opt.level++;
  return (opt.prefix  && opt.prefix(node,  opt.prefixData, opt.level - 1)) ||
         nodeTraverse(node->left,  opt) ||
         (opt.infix   && opt.infix(node,   opt.infixData, opt.level - 1)) ||
         nodeTraverse(node->right, opt) ||
         (opt.postfix && opt.postfix(node, opt.postfixData, opt.level - 1));
}

TreeNode* nodeCopy(TreeNode* src, TreeNode* newParent, Error* status) {
  if (!src)
    RETURN_WITH_STATUS(InvalidParameters, NULL);

  Error returnedStatus = OK;
  TreeNode* copy = nodeAlloc((NodeUnit){src->data.type, src->data.value}, newParent,
                                   NULL, NULL, &returnedStatus);
  if (returnedStatus) {
    nodeDestroy(copy);
    RETURN_WITH_STATUS(returnedStatus, NULL);
  }

  if (src->left)
    copy->left = nodeCopy(src->left, copy, &returnedStatus);
  if (src->right)
    copy->right = nodeCopy(src->right, copy, &returnedStatus);

  if (returnedStatus) {
    nodeDestroy(copy);
    RETURN_WITH_STATUS(returnedStatus, NULL);
  }

  return copy;
}

void nodeFixParents(TreeNode* node) {
  if (!node)
    return;

  if (node->left) {
    node->left->parent  = node;
    nodeFixParents(node->left);
  }
  if (node->right) {
    node->right->parent = node;
    nodeFixParents(node->right);
  }
}

Error nodeChangeChild(TreeNode* parent, TreeNode* child, 
                      TreeNode* newChild, size_t* nodeCount) {
  TreeNode** childPath = NULL;
  if (!parent) {
    if (newChild)
      newChild->parent = parent;
    nodeDestroyC(child, nodeCount);
  } else if (*(childPath = &parent->left)  == child ||
             *(childPath = &parent->right) == child) {
    *childPath = newChild;
    if (newChild)
      newChild->parent = parent;
    nodeDestroyC(child, nodeCount);
  }

  return OK;
}

Error nodeOptimize(TreeNode** node) {
  if (!node ||
      !*node)
    return InvalidParameters;

  Error returnedStatus = OK;
  TreeRoot* root = attachRoot(*node, &returnedStatus);
  if (returnedStatus)
    return returnedStatus;
  size_t prevNodeCount = 0;
  do {
    prevNodeCount = root->nodeCount;
    nodeOptimizeConstants(*node, &root->nodeCount, NULL);
    nodeOptimizeNeutral(node, &root->nodeCount);
    nodeFixParents(*node);
  } while (prevNodeCount != root->nodeCount);

  detachRoot(root, &returnedStatus);
  if (returnedStatus)
    return returnedStatus;
  return OK;
}

static double nodeOptimizeConstants(TreeNode* node, size_t* nodeCount, Error* status) {
  if (!node)
    RETURN_WITH_STATUS(InvalidParameters, NAN);
  if (!IS_OP(node))
    RETURN_WITH_STATUS(OK, NAN);
  bool suppressOptimization = (OF_OP(node->parent, OP_POW) &&
                               OF_OP(node, OP_DIV) &&
                               OF_NUM(node->left, 1));

  OpType opType = node->data.value.op;
  const OpTypeInfo* i = parseOpType(opType);
  assert(i);
  switch (i->argCount) {
    case 1: {
      double rightVal = IS_NUM(node->right)
                        ? node->right->data.value.num
                        : nodeOptimizeConstants(node->right, nodeCount, status);
      if (!suppressOptimization &&
          !node->left &&
          !isnan(rightVal)) {
        double result = applyOperation(opType, rightVal, NAN);
        nodeDeleteC(node->right, nodeCount);
        node->data.type = NUM_TYPE;
        node->data.value.num = result;
        return result;
      }
      return NAN;
    }
    case 2: {
      double leftVal =  IS_NUM(node->left)
                        ? node->left->data.value.num
                        : nodeOptimizeConstants(node->left, nodeCount, status);
      double rightVal = IS_NUM(node->right)
                        ? node->right->data.value.num
                        : nodeOptimizeConstants(node->right, nodeCount, status);
      if (!suppressOptimization &&
          !isnan(leftVal) &&
          !isnan(rightVal)) {
        double result = applyOperation(opType, leftVal, rightVal);
        nodeDeleteC(node->left,  nodeCount);
        nodeDeleteC(node->right, nodeCount);
        node->data.type = NUM_TYPE;
        node->data.value.num = result;
        return result;
      }
      return NAN;
    }
    default:
      return NAN;
  }

  return NAN;
}

#define REPLACE_WITH(newNode)                          \
  {                                                    \
  TreeNode* newChild = newNode;                        \
  newNode = NULL;                                      \
  TreeNode* parent = (*node)->parent;                  \
  nodeChangeChild(parent, *node, newChild, nodeCount); \
  *node = newChild;                                    \
  }                                                    \

#define REDUCE_TO_NUM(nodeValue)               \
  {                                            \
  nodeDeleteC((*node)->left , nodeCount);      \
  nodeDeleteC((*node)->right, nodeCount);      \
  (*node)->data.type      = NUM_TYPE;          \
  (*node)->data.value.num = nodeValue;         \
  }

static Error nodeOptimizeNeutral(TreeNode** node, size_t* nodeCount) {
  if (!node ||
      !*node)
    return InvalidParameters;
  if (!IS_OP(*node))
    return OK;

  nodeOptimizeNeutral(&(*node)->left, nodeCount);
  nodeOptimizeNeutral(&(*node)->right, nodeCount);

  switch ((*node)->data.value.op) {
    case OP_MUL: {
      if (OF_NUM((*node)->left, 0) ||
          OF_NUM((*node)->right, 0)) {
        REDUCE_TO_NUM(0);
      } else if (OF_NUM((*node)->left, 1)) {
        REPLACE_WITH((*node)->right);
      } else if (OF_NUM((*node)->right, 1)) {
        REPLACE_WITH((*node)->left);
      }
      break;
    }
    case OP_DIV: {
      if (OF_NUM((*node)->left, 0)) {
        REDUCE_TO_NUM(0);
      } else if (OF_NUM((*node)->right, 1)) {
        REPLACE_WITH((*node)->left);
      }
      break;
    }
    case OP_ADD: {
      if (OF_NUM((*node)->left, 0)) {
        REPLACE_WITH((*node)->right);
      } else if (OF_NUM((*node)->right, 0)) {
        REPLACE_WITH((*node)->left);
      }
      break;
    }
    case OP_SUB: {
      if (OF_NUM((*node)->right, 0))
        REPLACE_WITH((*node)->left);
      break;
    }
    case OP_POW: {
      if (OF_NUM((*node)->right, 0) ||
          OF_NUM((*node)->left, 1)) {
        REDUCE_TO_NUM(1);
      } else if (OF_NUM((*node)->left, 0)) {
        REDUCE_TO_NUM(0);
      } else if (OF_NUM((*node)->right, 1)) {
        REPLACE_WITH((*node)->left);
      }
      break;
    }
    default:
      break;
  }

  return OK;
}

#undef REPLACE_WITH
#undef REDUCE_TO_NUM

Error nodeDeleteC(TreeNode* node, size_t* nodeCount) {
  if (!node)
    return InvalidParameters;

  if (node->parent) {
    if (node->parent->left == node)
      node->parent->left = NULL;
    else if (node->parent->right == node)
      node->parent->right = NULL;
  }

  return nodeDestroyC(node, nodeCount);
}

Error nodeDestroyC(TreeNode* node, size_t* nodeCount) {
  if (!node)
    return InvalidParameters;

  nodeDestroyC(node->left, nodeCount);
  nodeDestroyC(node->right, nodeCount);
  node->left   = NULL;
  node->right  = NULL;
  node->parent = NULL;
  node->data   = (NodeUnit){};

  free(node);
  if (nodeCount)
    (*nodeCount)--;

  return OK;
}

Error countNodesCallback(unused TreeNode* node, 
                         void* data, 
                         unused uint level) {
  if (!data)
    return InvalidParameters;
  size_t* nodeCount = (size_t*)data;
  (*nodeCount)++;
  return OK;
}

// here non-zero return is treated as found variable
Error findVariableCallback(TreeNode* node, void* data, 
                           unused uint level) {
  if (data)
    return OK; //nothing to find

  size_t* index = (size_t*)data;
  if (IS_VAR(node) 
      && node->data.value.var == *index)
    return 1;
  return OK;
}

#undef nodeAlloc

TreeNode* nodeAlloc(NodeAllocOpt opt) {
  TreeNode* node = (TreeNode*)calloc(1, sizeof(TreeNode));
  Error* status = opt.status;
  if (!node) {
    free(node);
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);
  }
  
  //так как NodeAllocOpt повторяет те же поля что и TreeNode, можем сделать так
  return memcpy(node, &opt, sizeof(TreeNode));
}

#undef RETURN_WITH_STATUS
