#include "diff/derivative.h"
#include "diff/io/io.h"
#include <assert.h>
#include <math.h>

#define D_CONST \
        NUM_(0)
#define D_X \
        NUM_(1)
#define D_(d) \
        differentiateRec(ctx, d, var)
#define D_L \
        D_(node->left)
#define D_R \
        D_(node->right)
#define C_(c) \
        nodeCopy(c, NULL, NULL)
#define C_L \
        C_(node->left)
#define C_R \
        C_(node->right)
#define CHAIN_RULE_R(l) \
        MUL_(l, D_R)
#define CHAIN_RULE_L(l) \
        MUL_(l, D_L)

static TreeNode* differentiateRec(Context* ctx, TreeNode* node, const char* var);
static TreeNode* differentiatePower(Context* ctx, TreeNode* node, const char* var);

#define DUMP_TO_TEX_AND_RETURN(returnNode)                                       \
        return ctx->sink                                                         \
               ? differentiationStepToTex(ctx, var, node, returnNode, NULL)      \
               : returnNode;

TreeNode* differentiate(Context* ctx, TreeNode* node, const char* var) {
  if (!node ||
      !var  ||
      !ctx)
    return NULL;

  if (ctx->sink) {
    if (contextVerify(ctx))
      return NULL;
    fputs("A derivative of this expression is deemed quite trivial:\\\\", ctx->sink);
    nodeToTex(ctx, node);
  }

  //if the var is not found, that means that the entire expression will diff-te to 0
  if (!findVar(ctx->vars, var, NULL, NULL))
    DUMP_TO_TEX_AND_RETURN(D_CONST);

  if (varsVerify(ctx->vars))
    return NULL;
  TreeNode* diff = differentiateRec(ctx, node, var);
  nodeFixParents(diff);
  return diff;
}

static TreeNode* differentiateRec(Context* ctx, TreeNode* node, const char* var) {
  assert(ctx && !varsVerify(ctx->vars));
  if (!node)
    return NULL;

  if (IS_NUM(node) || 
      (IS_VAR(node) && 
       !OF_VAR(ctx->vars, node, var))) {
    DUMP_TO_TEX_AND_RETURN(D_CONST);
  }

  if (OF_VAR(ctx->vars, node, var)) {
    DUMP_TO_TEX_AND_RETURN(D_X);
  }

  if (IS_OP(node)) {
    switch (node->data.value.op) {
      case OP_ADD:  DUMP_TO_TEX_AND_RETURN(ADD_(D_L, D_R));
      case OP_SUB:  DUMP_TO_TEX_AND_RETURN(SUB_(D_L, D_R));
      case OP_MUL:  DUMP_TO_TEX_AND_RETURN(ADD_(MUL_(D_L, C_R), MUL_(C_L, D_R)));
      case OP_DIV:  DUMP_TO_TEX_AND_RETURN(DIV_(SUB_(MUL_(C_R, D_L), MUL_(D_R, C_L)), SQ_(C_R)));
      case OP_POW:  DUMP_TO_TEX_AND_RETURN(differentiatePower(ctx, node, var));
      case OP_SIN:  DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(COS_(C_R)));
      case OP_COS:  DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(NEG_(SIN_(C_R))));
      case OP_TAN:  DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(INV_(SQ_(COS_(C_R)))));
      case OP_COT:  DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(NEG_INV_(SQ_(SIN_(C_R)))));
      case OP_LOG:  DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(INV_(MUL_(C_R, LN_(C_L)))));
      case OP_LN :  DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(INV_(C_R)));
      case OP_SINH: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(COSH_(C_R)));
      case OP_COSH: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(SINH_(C_R)));
      case OP_TANH: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(INV_(SQ_(COSH_(C_R)))));
      case OP_COTH: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(NEG_INV_(SQ_(SINH_(C_R)))));
      case OP_ASIN: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(INV_(SQRT_(SUB_(NUM_(1), SQ_(C_R))))));
      case OP_ACOS: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(NEG_INV_(SQRT_(SUB_(NUM_(1), SQ_(C_R))))));
      case OP_ATAN: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(INV_(ADD_(NUM_(1), SQ_(C_R)))));
      case OP_ACOT: DUMP_TO_TEX_AND_RETURN(CHAIN_RULE_R(NEG_INV_(ADD_(NUM_(1), SQ_(C_R)))));
      default: return NULL;
    }
  }

  return NULL;
}

#undef DUMP_TO_TEX_AND_RETURN

static TreeNode* differentiatePower(Context* ctx, TreeNode* node, const char* var) {
  assert(ctx && !varsVerify(ctx->vars));
  if (!node ||
      !node->left ||
      !node->right)
    return NULL;

  Error err = OK;
  size_t data = 0; //to store the index when successfully found
  /*Variable* v = */ findVar(ctx->vars, var, &err, &data);
  if (err) //UnknownVariable or other error
    return D_CONST;
  bool leftContainsX  = nodeTraverse(node->left, 
                                     .infix = findVariableCallback, 
                                     .infixData = (void*)&data);
  bool rightContainsX = nodeTraverse(node->right, 
                                     .infix = findVariableCallback, 
                                     .infixData = (void*)&data);
  
  if (!leftContainsX &&
      !rightContainsX)
    return D_CONST; 

  if (leftContainsX &&
      !rightContainsX)
    return CHAIN_RULE_L(MUL_(C_R, POW_(C_L, (SUB_(C_R, NUM_(1))))));

  if (!leftContainsX &&
      rightContainsX) {
    if (OF_NUM(node->left, M_E))
      return CHAIN_RULE_R(C_(node));
    return CHAIN_RULE_R(MUL_(C_(node), LN_(C_L)));
  }

  if (leftContainsX &&
      rightContainsX) {
    TreeNode* temp = MUL_(C_R, LN_(C_L));
    nodeFixParents(temp);
    TreeNode* result = MUL_(C_(node), D_(temp));
    nodeDestroy(temp, true, NULL);
    return result;
  }
  return NULL;
}

#undef D_CONST
#undef D_X
#undef D_
#undef D_L
#undef D_R
#undef C_
#undef C_L
#undef C_R
