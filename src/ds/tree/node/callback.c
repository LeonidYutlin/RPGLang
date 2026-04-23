#include "ds/tree/node/callback.h"
#include "utils/utils.h"

Error countNodesCallback(_unused TreeNode* node, 
                         void* data, 
                         _unused uint level) {
  if (!data)
    return BadArgs;
  size_t* nodeCount = (size_t*)data;
  (*nodeCount)++;
  return OK;
}
