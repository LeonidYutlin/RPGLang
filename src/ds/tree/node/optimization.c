/* #include "ds/tree/node/optimization.h"
#include "ds/tree/root.h"
#include <assert.h>
#include <math.h>
#include "utils/utils.h"

static double nodeOptimizeConstants(TreeNode* node, size_t* nodeCount, Error* status);
static Error nodeOptimizeNeutral(TreeNode** node, size_t* nodeCount);

Error nodeOptimize(TreeNode** node) {
  if (!node ||
      !*node)
    return BadArgs;

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
    RETURN_WITH_STATUS(BadArgs, NAN);
  if (!IS_OP(node))
    RETURN_WITH_STATUS(OK, NAN);

  OpType opType = node->data.value.op;
  const OpTypeInfo* i = parseOpType(opType);
  assert(i);
  switch (i->argCount) {
    case 1: {
      int64_t rightVal = IS_NUM(node->right)
                        ? node->right->data.value.num
                        : nodeOptimizeConstants(node->right, nodeCount, status);
      if (!node->left &&
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
      if (!isnan(leftVal) &&
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
    return BadArgs;
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
    default:
      break;
  }

  return OK;
}

#undef REPLACE_WITH
#undef REDUCE_TO_NUM */
