#ifndef NODE_CALLBACK_H
#define NODE_CALLBACK_H

#include "ds/tree/node/node.h"
#include "error/error.h"

Error countNodesCallback(TreeNode* node, void* data, uint level);
Error findVariableCallback(TreeNode* node, void* data, uint level);

#endif
