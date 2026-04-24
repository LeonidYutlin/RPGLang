#ifndef NODE_TYPE_H
#define NODE_TYPE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

//TODO: add a new node type for different control flow nodes
#define NODE_TYPE_LIST()          \
  X(UNKNOWN_TYPE, "UNKNOWN TYPE") \
  X(OP_TYPE,      "OP")           \
  X(NUM_TYPE,     "NUM")          \
  X(VAR_TYPE,     "VAR")

typedef enum NodeType {
  #define X(enm, ...) enm,
  NODE_TYPE_LIST()
  #undef X
} NodeType;

typedef struct NodeTypeInfo {
  NodeType type;
  const char* str;
} NodeTypeInfo;

const NodeTypeInfo* parseNodeType(NodeType type);

//NOTE:
//X(enum, "str", argc, prior)
#define OP_TYPE_LIST()       \
  X(OP_IF,     "if",   2, 1) \
  X(OP_SEMIC, ";",     2, 1) \
  X(OP_ASG,  "=",      2, 1) \
  X(OP_ADD,  "+",      2, 1) \
  X(OP_SUB,  "-",      2, 1) \
  X(OP_MUL,  "*",      2, 2) \
  X(OP_DIV,  "/",      2, 2) \
  X(OP_POW,  "^",      2, 3) \
  X(OP_SIN,  "sin",    1, 3) \
  X(OP_COS,  "cos",    1, 3) \
  X(OP_TAN,  "tan",    1, 3) \
  X(OP_COT,  "cot",    1, 3) \
  X(OP_LOG,  "log",    2, 3) \
  X(OP_LN,   "ln",     1, 3) \
  X(OP_ASIN, "arcsin", 1, 3) \
  X(OP_ACOS, "arccos", 1, 3) \
  X(OP_ATAN, "arctan", 1, 3) \
  X(OP_ACOT, "arccot", 1, 3) \
  X(OP_SINH, "sinh",   1, 3) \
  X(OP_COSH, "cosh",   1, 3) \
  X(OP_TANH, "tanh",   1, 3) \
  X(OP_COTH, "coth",   1, 3)

typedef enum OpType {
  #define X(enm, ...) enm,
  OP_TYPE_LIST()
  #undef X
} OpType;

typedef struct OpTypeInfo {
  OpType type;
  const char* str;
  uint argCount;
  uint priority;
} OpTypeInfo;

const OpTypeInfo* parseOpType(OpType type);
int getOpType(const char* string);
///Applies appropriate operation regarding a and b and returns the result.
///If the operation doesn't require a second parameter (e.g. cos(x)) then leave b as NAN
double applyOperation(OpType type, double a, double b);

//Node utility macros
#define IS_OP(node)  ((node) && (node)->data.type == OP_TYPE)
#define IS_NUM(node) ((node) && (node)->data.type == NUM_TYPE)
#define IS_VAR(node) ((node) && (node)->data.type == VAR_TYPE)
#define OF_OP(node, opType) \
  (IS_OP((node)) && (node)->data.value.op == (opType))
#define OF_NUM(node, i) \
  (IS_NUM((node)) && doubleEqual((node)->data.value.num, (i)))
//OF_VAR is in context.h because it's more finnicky

//Quick node initializers
#define OP_UNIT_(i)   (NodeUnit){.type = OP_TYPE,  .value = {.op = i}}
#define NUM_UNIT_(i)  (NodeUnit){.type = NUM_TYPE, .value = {.num = i}}
#define VAR_UNIT_(name, len)  (NodeUnit){.type = VAR_TYPE, .value = {.var = (StringView){name, len}}}

#define NUM_(i) nodeAlloc(NUM_UNIT_(i))
#define VAR_(name, len) nodeAlloc(VAR_UNIT_(name, len))

#define nodeAllocBinop(op, l, r) \
  nodeAlloc(.data = OP_UNIT_(op), .left = l, .right = r)
#define nodeAllocUnop(op, r) \
  nodeAllocBinop(op, NULL, r)

#define SEMIC_(l) \
        nodeAllocBinop(OP_SEMIC, l, NULL)
#define ASG_(l, r) \
        nodeAllocBinop(OP_ASG, l, r)
#define ADD_(l, r) \
        nodeAllocBinop(OP_ADD, l, r)
#define SUB_(l, r) \
        nodeAllocBinop(OP_SUB, l, r)
#define MUL_(l, r) \
        nodeAllocBinop(OP_MUL, l, r)
#define DIV_(l, r) \
        nodeAllocBinop(OP_DIV, l, r)
#define POW_(l, r) \
        nodeAllocBinop(OP_POW, l, r)
#define SQ_(l) \
        POW_(l, NUM_(2))
#define NEG_(r) \
        nodeAllocBinop(OP_MUL, NUM_(-1), r)
#define INV_(r) \
        nodeAllocBinop(OP_DIV, NUM_(1), r)
#define NEG_INV_(r) \
        nodeAllocBinop(OP_DIV, NUM_(-1), r)
#define SIN_(r) \
        nodeAllocUnop(OP_SIN, r)
#define COS_(r) \
        nodeAllocUnop(OP_COS, r)
#define LN_(r) \
        nodeAllocUnop(OP_LN, r)
#define SINH_(r) \
        nodeAllocUnop(OP_SINH, r)
#define COSH_(r) \
        nodeAllocUnop(OP_COSH, r)
#define SQRT_(l) \
        nodeAllocBinop(OP_POW, l, INV_(NUM_(2)))

#endif
