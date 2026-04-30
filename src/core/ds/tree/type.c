#include "ds/tree/type.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "utils/utils.h"

static const char* NODE_TYPES[] = {
  #define X(enm, s) [enm] = s,
  NODE_TYPE_LIST()
  #undef X
};

const size_t NODE_TYPES_SIZE = sizer(NODE_TYPES);

//TODO: maybe use _Generic for all get...Str and parse... enum things
const char* getNodeTypeStr(NodeType t) {
  return (t < 0 || (size_t)t >= NODE_TYPES_SIZE)
         ? NULL
         : NODE_TYPES[t];
}

int getNodeType(const char* str, size_t n) {
  for (int i = 0; (size_t)i < NODE_TYPES_SIZE; i++) {
    const char* t = getNodeTypeStr((NodeType)i);
    if (!t) continue;
    if (strncmp(str, t, n) == 0)
      return i;
  }
  return -1;
}

static const OpTypeInfo OP_TYPES[] = {
  #define X(enm, s, aC, pr)                 \
    [enm] = {.type = enm,                   \
             .str = s,                      \
             .argCount = aC,                \
             .priority = pr,                \
             },                                                   
  
  OP_TYPE_LIST()
  #undef X
};

static const size_t OP_TYPES_SIZE = sizer(OP_TYPES);

const OpTypeInfo* parseOpType(OpType t) {
  return (t < 0 || (size_t)t >= OP_TYPES_SIZE)
         ? NULL
         : &OP_TYPES[t];
}

//PERF: I know this is linear. I dont care. 
//Better than manual written cmps and easier than a new dt struct
int getOpType(const char* str, size_t n) {
  for (int i = 0; (size_t)i < OP_TYPES_SIZE; i++) {
    const OpTypeInfo* t = parseOpType((OpType)i);
    if (!t) continue;
    if ((t->str && strncmp(str, t->str, n) == 0)) 
      return (OpType)i;
  }
  return -1;
}

int64_t applyOperation(OpType type, int64_t a, int64_t b, Error* status) {
  switch (type) {
    case OP_ADD:  return a + b;
    case OP_SUB:  return a - b;
    case OP_MUL:  return a * b;
    case OP_DIV:  return a / b;
    case OP_SHL:  return a << b;
    case OP_SHR:  return a >> b;
    case OP_GRT:  return a > b;
    case OP_LSR:  return a < b;
    case OP_EQ :  return a == b;
    case OP_NEQ:  return a != b;
    case OP_AND:  return a && b;
    case OP_OR :  return a || b;
    case OP_NOT:  return !a;
    default:  
      assert(0 && "unreachable applyOperation");
      RETURN_WITH_STATUS(Fail, 0);
  }
}

static const char* CTRL_TYPES[] = {
  #define X(enm, str) [enm] = str,
  CTRL_TYPE_LIST()
  #undef X
};

static const size_t CTRL_TYPES_SIZE = sizer(CTRL_TYPES);

const char* getCtrlTypeStr(CtrlType t) {
  return (t < 0 || (size_t)t >= CTRL_TYPES_SIZE)
         ? NULL
         : CTRL_TYPES[t];
}

int getCtrlType(const char* str, size_t n) {
  for (int i = 0; (size_t)i < CTRL_TYPES_SIZE; i++) {
    const char* t = getCtrlTypeStr((CtrlType)i);
    if (!t) continue;
    if (strncmp(str, t, n) == 0)
      return i;
  }
  return -1;
}

static const char* VAR_TYPES[] = {
  #define X(enm, str) [enm] = str,
  VAR_TYPE_LIST()
  #undef X
};

static const size_t VAR_TYPES_SIZE = sizer(VAR_TYPES);

const char* getVarTypeStr(VarType t) {
  return (t < 0 || (size_t)t >= VAR_TYPES_SIZE)
         ? NULL
         : VAR_TYPES[t];
}

int getVarTypeType(const char* str, size_t n) {
  for (int i = 0; (size_t)i < VAR_TYPES_SIZE; i++) {
    const char* t = getVarTypeStr((VarType)i);
    if (!t) continue;
    if (strncmp(str, t, n) == 0)
      return i;
  }
  return -1;
}
