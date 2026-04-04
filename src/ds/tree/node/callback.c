#include "ds/tree/node/callback.h"
#include "utils/utils.h"

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
