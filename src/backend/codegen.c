#include "backend/codegen.h"
#include <assert.h>
#include <stdarg.h>

typedef struct {
  FILE* sink;
  uint64_t labelCount;
  uint64_t depth;
} Context;

// each register is 8 bytes
static const size_t REG_SIZE = 8;
static const char* const ARG_REGS[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};
static const size_t ARG_REGS_SIZE = sizer(ARG_REGS);

static void codegenRec(Context* ctx, TreeNode* ast, uint64_t endLabel);
static inline void push(Context* ctx, TreeNode* ast);
static inline void op(Context* ctx, TreeNode* ast);
static inline void not(Context* ctx, TreeNode* ast);
static inline void or(Context* ctx, TreeNode* ast);
static inline void and(Context* ctx, TreeNode* ast);
static inline void ctrl(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static inline void handleIfBranches(Context* ctx, TreeNode* ast, 
                                    uint64_t label, uint64_t* newLabel);
static inline void call(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static void cmp(Context* ctx, TreeNode* ast, const char* cmpStr);
static void clearStack(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static void gen_(FILE* sink, const char* commentary, 
                 const char* fmt, ...) _format(printf, 3, 4);

#define sinkFile sink
#define gen(commentary, fmt, ...) \
  gen_(sinkFile, "; -- " commentary " --\n", fmt __VA_OPT__(,) __VA_ARGS__)
#define genn(fmt, ...) \
  gen_(sinkFile, "", fmt __VA_OPT__(,) __VA_ARGS__)
#ifdef BACKEND_DEBUG_INFO
#define com(commentary) fputs("; -- " commentary " --\n", sinkFile)
#else
#define com(commentary)
#endif

// TODO: backend for: 
// CTRL: CTRL_ASG,
//   CTRL_WHILE, CTRL_UNTIL, CTRL_DECL, CTRL_PARAM, CTRL_FUNC_DECL,
//   CTRL_SIGNATURE, CTRL_RETURN, CTRL_CONTINUE, CTRL_BREAK
// IDENT: yeah
// TYPE: frac and loc

void codegen(FILE* sink, TreeNode* ast) {
  if (!sink || !ast)
    return;

  gen("HEADER",
      "global _start\n"
      "extern rout\n"
      "extern out\n"
      "extern exit\n"
      "section .text\n"
      "_start:\n");

  Context ctx = (Context){
    .sink = sink,
    .labelCount = 0,
    .depth = 0,
  };
  codegenRec(&ctx, ast, 0);
  gen("EXIT",
      "\t\tmov rax, 0x3c ; syscall exit\n"
      "\t\tmov rdi, 123\n"
      "\t\tsyscall\n");
}

#undef sinkFile
#define sinkFile ctx->sink

static void codegenRec(Context* ctx, TreeNode* ast, uint64_t endLabel) {
  if (!ctx || 
      !ctx->sink ||
      !ast)
    return;

  uint64_t oldDepth = ctx->depth;

  codegenRec(ctx, ast->left, 0);

  uint64_t rightEndLabel = 0;
  handleIfBranches(ctx, ast, endLabel, &rightEndLabel); 

  codegenRec(ctx, ast->right, rightEndLabel);


  if (IS_NUM(ast)) {
    push(ctx, ast);    
    return;
  }
  
  if (IS_OP(ast)) {
    op(ctx, ast); 
    return;
  }

  if (IS_CTRL(ast)) {
    ctrl(ctx, ast, oldDepth);
    return;
  }
}

#define PRELUDE()    \
  assert(ctx);       \
  assert(ctx->sink); \
  assert(ast);

static void push(Context* ctx, TreeNode* ast) {
  PRELUDE();
  gen("PUSH",
      "\t\tpush %ld\n", 
      ast->data.value.num);
  ctx->depth++;
}

static void op(Context* ctx, TreeNode* ast) {
  PRELUDE();
  if (ast->left) {
    genn("\t\tpop rbx\n");
    ctx->depth--;
  }
  if (ast->right) {
    genn("\t\tpop rax\n");
    ctx->depth--;
  }
  switch (ast->data.value.op) {
    case OP_ADD:
      gen("ADD",
          "\t\tadd rax, rbx\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_SUB:
      gen("SUB",
          "\t\tsub rax, rbx\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_MUL:
      gen("MUL",
          "\t\timul rax, rbx\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_DIV:
      gen("DIV",
          "\t\tcqo\n"
          "\t\tidiv rbx\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_SHL:
      gen("SHL",
          "\t\tmov cl, bl\n"
          "\t\tshl rax, cl\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_SHR:
      gen("SHR",
          "\t\tmov cl, bl\n"
          "\t\tshr rax, cl\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_NOT:
      not(ctx, ast);
      break;
    case OP_AND:
      and(ctx, ast);
      break;
    case OP_OR:
      or(ctx, ast); 
      break;
#ifdef CONDITIONAL_MOVES
    case OP_GRT:
      com("GRT");
      cmp(ctx, ast, "cmovg");
      break;
    case OP_LSR:
      com("LSR");
      cmp(ctx, ast, "cmovl");
      break;    
    case OP_EQ:
      com("EQ");
      cmp(ctx, ast, "cmove");
      break;    
    case OP_NEQ:
      com("NEQ");
      cmp(ctx, ast, "cmovne");
      break;
#else
    case OP_GRT:
      com("GRT");
      cmp(ctx, ast, "jg");
      break;
    case OP_LSR:
      com("LSR");
      cmp(ctx, ast, "jl");
      break;
    case OP_EQ:
      com("EQ");
      cmp(ctx, ast, "je");
      break;
    case OP_NEQ:
      com("NEQ");
      cmp(ctx, ast, "jne");
      break;
#endif
    default: break;
  }
}

static void ctrl(Context* ctx, TreeNode* ast, uint64_t oldDepth) {
  PRELUDE();
  switch (ast->data.value.ctrl) {
    case CTRL_SEMIC: 
      clearStack(ctx, ast, oldDepth);
      break;
    case CTRL_FUNC_CALL:
      call(ctx, ast, oldDepth);
      break;
    default: break;
  }
}

static void handleIfBranches(Context* ctx, TreeNode* ast, 
                             uint64_t label, uint64_t* newLabel) {
  PRELUDE();
  assert(newLabel);
  if (OF_CTRL(ast, CTRL_IF)) {
    *newLabel = ctx->labelCount++;
    gen("IF",
        "\t\tpop rax\n"
        "\t\ttest rax, rax\n"
        "\t\tjz .if_end%zu\n",
        *newLabel);
    ctx->depth--;
  } else if (OF_CTRL(ast, CTRL_ELSE)) {
    genn(".else_end%zu:\n", label);
  } else if (OF_CTRL(ast->parent, CTRL_IF) &&
             ast->parent->right == ast) {
    if (OF_CTRL(ast->right, CTRL_ELSE)) {
      *newLabel = ctx->labelCount++;
      genn("\t\tjmp .else_end%zu\n", *newLabel);
    }
    genn(".if_end%zu:\n", label);
  }
}

static void clearStack(Context* ctx, _unused TreeNode* ast, uint64_t oldDepth) {
  PRELUDE();
  if (ctx->depth != oldDepth) {
      gen("CLEAR STACK",
          "\t\tadd rsp, %zu\n",
          (ctx->depth - oldDepth) * REG_SIZE);
      ctx->depth = oldDepth;
  }
}

static void call(Context* ctx, TreeNode* ast, uint64_t oldDepth) {
  PRELUDE();

  com("CALL");
  size_t i = 0;
  for (TreeNode* arg = ast->right; 
       arg && i < ARG_REGS_SIZE; arg = arg->right) {
    genn("\t\tpop %s\n",
         ARG_REGS[i]);
    ctx->depth--;
    i++;
  }

  StringView funcName = ast->left->data.value.id;
  genn("\t\tcall %.*s\n",
       (int)funcName.size, funcName.data);
  clearStack(ctx, ast, oldDepth); 
}

static void cmp(Context* ctx, TreeNode* ast, const char* cmpStr) {
  PRELUDE();
  assert(cmpStr);
#ifdef CONDITIONAL_MOVES
  genn("\t\tmov rcx, rax\n"
       "\t\txor eax, eax\n"
       "\t\tcmp rcx, rbx\n"
       "\t\tmov rcx, 1\n"
       "\t\t%s rax, rcx\n"
       "\t\tpush rax\n",
       cmpStr);
#else
  uint64_t pushZeroLabel     = ctx->labelCount++;
  uint64_t skipPushZeroLabel = ctx->labelCount++;
  genn("\t\tcmp rax, rbx\n"
       "\t\t%s .push_one%zu\n"
       //"\t\t.push_zero%zu:\n"
       "\t\tpush 0\n"
       "\t\tjmp .skip_push_one%zu\n"
       ".push_one%zu:\n"
       "\t\tpush 1\n"
       ".skip_push_one%zu:\n",
       cmpStr,
       pushZeroLabel, skipPushZeroLabel,
       pushZeroLabel, skipPushZeroLabel);
#endif
  ctx->depth++;
}

static void not(Context* ctx, TreeNode* ast) {
  PRELUDE();
#ifdef CONDITIONAL_MOVES
  gen("LOGICAL NOT",
      "\t\tmov rcx, rax\n"
      "\t\txor eax, eax\n"
      "\t\ttest rcx, rcx\n"
      "\t\tmov rcx, 1\n"
      "\t\tcmovz rax, rcx\n"
      "\t\tpush rax\n");
#else
  uint64_t pushZeroLabel     = ctx->labelCount++;
  uint64_t skipPushZeroLabel = ctx->labelCount++;
  gen("LOGICAL NOT",
      "\t\ttest rax, rax\n"
      "\t\tjz .push_one%zu\n"
      "\t\tpush 0\n"
      "\t\tjmp .skip_push_one%zu\n"
      ".push_one%zu:\n"
      "\t\tpush 1\n"
      ".skip_push_one%zu:\n",
      pushZeroLabel, skipPushZeroLabel,
      pushZeroLabel, skipPushZeroLabel);    
#endif
  ctx->depth++;
}

static void and(Context* ctx, TreeNode* ast) {
  PRELUDE();
  uint64_t falseLabel     = ctx->labelCount++;
  uint64_t skipFalseLabel = ctx->labelCount++;
  gen("LOGICAL AND",
      "\t\ttest rax, rax\n"
      "\t\tjz .false%zu\n"
      "\t\ttest rbx, rbx\n"
      "\t\tjz .false%zu\n"
      "\t\tpush 1\n"
      "\t\tjmp .skip_false%zu\n"
      ".false%zu:\n"
      "\t\tpush 0\n"
      ".skip_false%zu:\n",
      falseLabel,     falseLabel,
      skipFalseLabel, falseLabel,
      skipFalseLabel);    
  ctx->depth++;
}

static void or(Context* ctx, TreeNode* ast) {
  PRELUDE();
  uint64_t trueLabel     = ctx->labelCount++;
  uint64_t skipTrueLabel = ctx->labelCount++;
  gen("LOGICAL OR",
      "\t\ttest rax, rax\n"
      "\t\tjnz .true%zu\n"
      "\t\ttest rbx, rbx\n"
      "\t\tjnz .true%zu\n"
      "\t\tpush 0\n"
      "\t\tjmp .skip_true%zu\n"
      ".true%zu:\n"
      "\t\tpush 1\n"
      ".skip_true%zu:\n",
      trueLabel,     trueLabel,
      skipTrueLabel, trueLabel,
      skipTrueLabel);    
  ctx->depth++;
}

#undef sinkFile
#undef gen
#undef genn
#undef com
#undef PRELUDE

static void gen_(FILE* sink, const char* commentary, 
                 const char* fmt, ...) {
  assert(sink);
  assert(commentary);
  assert(fmt);

  #ifdef BACKEND_DEBUG_INFO
  fputs(commentary, sink);
  #endif

  va_list args = {};
  va_start(args, fmt);
  vfprintf(sink, fmt, args);
  va_end(args);
}
