#include "ds/tree/nodetype.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "misc/util.h"

static const NodeTypeInfo NODE_TYPES[] = {
  #define X(enm, s)       \
    [enm] = {.type = enm, \
             .str = s},                                                   
  
  NODE_TYPE_LIST()
  #undef X
};

const size_t NODE_TYPES_SIZE = sizer(NODE_TYPES);

const NodeTypeInfo* parseNodeType(NodeType t) {
  return (t < 0 || (size_t)t >= NODE_TYPES_SIZE)
         ? NULL
         : &NODE_TYPES[t];
}

static const OpTypeInfo OP_TYPES[] = {
  #define X(enm, s, aS, aC, pr, supp)       \
    [enm] = {.type = enm,                   \
             .str = s,                      \
             .alt = aS,                     \
             .argCount = aC,                \
             .priority = pr,                \
             .isSupported = supp},                                                   
  
  OP_TYPE_LIST()
  #undef X
};

const size_t OP_TYPES_SIZE = sizer(OP_TYPES);

const OpTypeInfo* parseOpType(OpType t) {
  return (t < 0 || (size_t)t >= OP_TYPES_SIZE)
         ? NULL
         : &OP_TYPES[t];
}

//PERF: I know this is linear. I dont care. 
//Better than manual written cmps and easier than a new dt struct
int getOpType(const char* str) {
  for (int i = 0; (size_t)i < OP_TYPES_SIZE; i++) {
    const OpTypeInfo* t = parseOpType((OpType)i);
    if (!t) continue;
    if ((t->str && strcmp(str, t->str) == 0) || 
        (t->alt && strcmp(str, t->alt) == 0)) 
      return (OpType)i;
  }
  return -1;
}

double applyOperation(OpType type, double a, double b) {
  switch (type) {
    case OP_ADD:  return a + b;
    case OP_SUB:  return a - b;
    case OP_MUL:  return a * b;
    case OP_DIV:  return a / b;
    case OP_POW:  return pow(a, b);
    case OP_SIN:  return sin(a);
    case OP_COS:  return cos(a);
    case OP_TAN:  return tan(a);
    case OP_COT:  return 1 / tan(a);
    case OP_ASIN: return asin(a);
    case OP_ACOS: return acos(a);
    case OP_ATAN: return atan(a);
    case OP_ACOT: return M_PI_2 - atan(a);
    case OP_SINH: return sinh(a);
    case OP_COSH: return cosh(a);
    case OP_TANH: return tanh(a);
    case OP_COTH: return 1 / tanh(a);
    case OP_LOG:  return log(b) / log(a);
    case OP_LN :  return log(a);
    default:      return NAN;
  }
}
