#ifndef NODE_TYPE_H
#define NODE_TYPE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define NODE_TYPE_LIST()           \
  X(UNKNOWN_TYPE,  "UNKNOWN TYPE") \
  X(OP_TYPE,       "OP")           \
  X(CTRL_TYPE,     "CTRL")         \
  X(NUM_TYPE,      "NUM")          \
  X(IDENT_TYPE,    "IDENT")        \
  X(VAR_TYPE_TYPE, "TYPE")

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
//X(enum, "str")
#define VAR_TYPE_LIST() \
  X(TYPE_PRIM, "prim")  \
  X(TYPE_FRAC, "frac")  \
  X(TYPE_LOC,  "loc")   \
  X(TYPE_VOID, "void")

//NOTE:
//X(enum, "str")
#define CTRL_TYPE_LIST()         \
  X(CTRL_SEMIC,     ";")         \
  X(CTRL_ASG,       "=")         \
  X(CTRL_IF,        "if")        \
  X(CTRL_ELSE,      "else")      \
  X(CTRL_WHILE,     "while")     \
  X(CTRL_UNTIL,     "until")     \
  X(CTRL_DECL,      "decl")      \
  X(CTRL_PARAM,     "parameter") \
  X(CTRL_FUNC_DECL, "func decl") \
  X(CTRL_FUNC_CALL, "func call") \
  X(CTRL_SIGNATURE, "signature") \
  X(CTRL_RETURN,    "return")    \
  X(CTRL_CONTINUE,  "continue")  \
  X(CTRL_BREAK,     "break")

//TODO: priorities here are messed up; fix them
//NOTE:
//X(enum, "str", argc, prior)
#define OP_TYPE_LIST()       \
  X(OP_NOT,  "!",      1, 0) \
  X(OP_ADD,  "+",      2, 1) \
  X(OP_SUB,  "-",      2, 1) \
  X(OP_MUL,  "*",      2, 2) \
  X(OP_DIV,  "/",      2, 2) \
  X(OP_SHL,  "shl",    1, 3) \
  X(OP_SHR,  "shr",    1, 3) \
  X(OP_GRT,  "grt",    1, 4) \
  X(OP_LSR,  "lsr",    1, 4) \
  X(OP_EQ,   "==",     1, 5) \
  X(OP_NEQ,  "!=",     1, 5) \
  X(OP_AND,  "and",     1, 5) \
  X(OP_OR,   "or",     1, 5) \
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

typedef enum CtrlType {
  #define X(enm, ...) enm,
  CTRL_TYPE_LIST()
  #undef X
} CtrlType;

const char* getCtrlTypeStr(CtrlType type);

typedef enum VarType {
  #define X(enm, ...) enm,
  VAR_TYPE_LIST()
  #undef X
} VarType;

const char* getVarTypeStr(VarType type);

//Node utility macros
#define IS_TYPE(node, t) ((node) && (node)->data.type == t) 
#define IS_OP(node)    IS_TYPE(node, OP_TYPE)
#define IS_NUM(node)   IS_TYPE(node, NUM_TYPE)
#define IS_IDENT(node) IS_TYPE(node, IDENT_TYPE)
#define IS_CTRL(node)  IS_TYPE(node, CTRL_TYPE)
#define IS_VAR_TYPE(node) IS_TYPE(node, VAR_TYPE_TYPE)
#define OF_VAR_TYPE(node, varType) \
  (IS_VAR_TYPE((node)) && (node)->data.value.varType == (varType))
#define OF_CTRL(node, ctrlType) \
  (IS_CTRL((node)) && (node)->data.value.ctrl == (ctrlType))
#define OF_OP(node, opType) \
  (IS_OP((node)) && (node)->data.value.op == (opType))
#define OF_NUM(node, i) \
  (IS_NUM((node)) && doubleEqual((node)->data.value.num, (i)))

//Quick TreeNode and NodeUnit initializers
#define OP_UNIT_(i)   (NodeUnit){.type = OP_TYPE,   .value = {.op = i}}
#define CTRL_UNIT_(i) (NodeUnit){.type = CTRL_TYPE, .value = {.ctrl = i}}
#define NUM_UNIT_(i)  (NodeUnit){.type = NUM_TYPE,  .value = {.num = i}}
#define IDENT_UNIT_(name, len)  (NodeUnit){.type = IDENT_TYPE, .value = {.id = (StringView){name, len}}}
#define VAR_TYPE_UNIT_(i)  (NodeUnit){.type = VAR_TYPE_TYPE, .value = {.varType = i}}

#define NUM_(i) nodeAlloc(NUM_UNIT_(i))
#define IDENT_(name, len) nodeAlloc(IDENT_UNIT_(name, len))

#define PRIM_() nodeAlloc(VAR_TYPE_UNIT_(TYPE_PRIM))
#define FRAC_() nodeAlloc(VAR_TYPE_UNIT_(TYPE_FRAC))
#define LOC_()  nodeAlloc(VAR_TYPE_UNIT_(TYPE_LOC))
#define VOID_() nodeAlloc(VAR_TYPE_UNIT_(TYPE_VOID))

#define nodeAllocCtrl(ctrl, l, r) \
  nodeAlloc(.data = CTRL_UNIT_(ctrl), .left = l, .right = r)

#define SEMIC_(l) \
        nodeAllocCtrl(CTRL_SEMIC, l, NULL)
#define ASG_(l, r) \
        nodeAllocCtrl(CTRL_ASG, l, r)
#define IF_(l, r) \
        nodeAllocCtrl(CTRL_IF, l, r)
#define ELSE_(r) \
        nodeAllocCtrl(CTRL_ELSE, NULL, r)
#define DECL_(l, r) \
        nodeAllocCtrl(CTRL_DECL, l, r)
#define PARAM_(l, r) \
        nodeAllocCtrl(CTRL_PARAM, l, r)
#define FUNC_DECL_(l, r) \
        nodeAllocCtrl(CTRL_FUNC_DECL, l, r)
#define SIGNATURE_(l, r) \
        nodeAllocCtrl(CTRL_SIGNATURE, l, r)
#define FUNC_CALL_(l, r) \
        nodeAllocCtrl(CTRL_FUNC_CALL, l, r)
#define RETURN_(l) \
        nodeAllocCtrl(CTRL_RETURN, l, NULL)
#define BREAK_() \
        nodeAllocCtrl(CTRL_BREAK, NULL, NULL)
#define CONTINUE_() \
        nodeAllocCtrl(CTRL_CONTINUE, NULL, NULL)

#define nodeAllocBinop(op, l, r) \
  nodeAlloc(.data = OP_UNIT_(op), .left = l, .right = r)
#define nodeAllocUnop(op, r) \
  nodeAllocBinop(op, NULL, r)

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
#define NOT_(r) \
        nodeAllocUnop(OP_NOT, r)
#define AND_(l, r) \
        nodeAllocBinop(OP_AND, l, r)
#define OR_(l, r) \
        nodeAllocBinop(OP_OR, l, r)
#define GRT_(l, r) \
        nodeAllocBinop(OP_GRT, l, r)
#define LSR_(l, r) \
        nodeAllocBinop(OP_LSR, l, r)
#define EQ_(l, r) \
        nodeAllocBinop(OP_EQ, l, r)
#define NEQ_(l, r) \
        nodeAllocBinop(OP_NEQ, l, r)
#define SHR_(l, r) \
        nodeAllocBinop(OP_SHR, l, r)
#define SHL_(l, r) \
        nodeAllocBinop(OP_SHL, l, r)
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
