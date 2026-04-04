#ifndef DERIVATIVE_H
#define DERIVATIVE_H

#include "diff/context.h"
#include "ds/tree/node.h"
 
TreeNode* differentiate(Context* context, TreeNode* node, const char* var);

#endif
