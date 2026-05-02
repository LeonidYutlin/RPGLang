#include "backend/codegen.h"
#include <assert.h>
#include <stdarg.h>

typedef struct {
  FILE* sink;
  uint64_t labelCount;
  uint64_t depth;
  size_t regIndex;
} Context;

// each register is 8 bytes
static const size_t REG_SIZE = 8;
static const char* const ARG_REGS[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};
static const size_t ARG_REGS_SIZE = sizer(ARG_REGS);

static void codegenRec(Context* ctx, TreeNode* ast);
static inline void push(Context* ctx, TreeNode* ast);
static inline void op(Context* ctx, TreeNode* ast);
static inline void call(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static inline void arg(Context* ctx, TreeNode* ast);
static void clearStack(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static void gen_(FILE* sink, const char* commentary, 
                 const char* fmt, ...) _format(printf, 3, 4);

#define sinkFile sink
#define gen(commentary, fmt, ...) \
  gen_(sinkFile, "; -- " commentary " --\n", fmt __VA_OPT__(,) __VA_ARGS__)
#define genn(fmt, ...) \
  gen_(sinkFile, "", fmt __VA_OPT__(,) __VA_ARGS__)
#ifdef BACKEND_DEBUG_INFO
#define com(commentary) fputs("; -- " commentary " --\n", sink)
#else
#define com(commentary)
#endif

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
    .regIndex = 0,
  };
  codegenRec(&ctx, ast);
  gen("EXIT",
      "\t\tmov rax, 0x3c ; syscall exit\n"
      "\t\tmov rdi, 123\n"
      "\t\tsyscall\n");
}

#undef sinkFile
#define sinkFile ctx->sink

static void codegenRec(Context* ctx, TreeNode* ast) {
  if (!ctx || 
      !ctx->sink ||
      !ast)
    return;

  uint64_t oldDepth = ctx->depth;
  codegenRec(ctx, ast->left);
  codegenRec(ctx, ast->right);

  if (IS_NUM(ast)) {
    push(ctx, ast);    
    return;
  }
  
  if (IS_OP(ast)) {
    op(ctx, ast); 
    return;
  }

  if (OF_CTRL(ast, CTRL_SEMIC)) {
    clearStack(ctx, ast, oldDepth); 
    return;
  } 

  if (OF_CTRL(ast, CTRL_ARG)) {
    arg(ctx, ast); 
    return;
  }

  if (OF_CTRL(ast, CTRL_FUNC_CALL)) {
    call(ctx, ast, oldDepth);
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
  if (ast->left)
    genn("\t\tpop rbx\n");
  if (ast->right)
    genn("\t\tpop rax\n");
  switch (ast->data.value.op) {
    case OP_ADD:
      gen("ADD",
          "\t\tadd rax, rbx\n"
          "\t\tpush rax\n");
      ctx->depth--;
      break;
    case OP_SUB:
      gen("SUB",
          "\t\tsub rax, rbx\n"
          "\t\tpush rax\n");
      ctx->depth--;
      break;
    case OP_MUL:
      gen("MUL",
          "\t\timul rax, rbx\n"
          "\t\tpush rax\n");
      ctx->depth--;
      break;
    case OP_DIV:
      // NOTE: i'm not pleased with this implementation
      // alternative could be cmovz rdx, 0xFFFFFFFF
      gen("DIV",
          "\t\txor edx, edx\n"
          "\t\ttest rax, 0x80000000\n"
          "\t\tjz .no_sign_extension%zu\n"
          "\t\tdec rdx\n"
          "\t\t.no_sign_extension%zu:\n"
          "\t\tidiv rbx\n"
          "\t\tpush rax\n",
          ctx->labelCount, ctx->labelCount);
      ctx->labelCount++;
      ctx->depth--;
      break;
    default: break;
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
  StringView funcName = ast->left->data.value.id;
  gen("CALL",
      "\t\tcall %.*s\n",
      (int)funcName.size, funcName.data);
  clearStack(ctx, ast, oldDepth); 
  ctx->regIndex = 0;
}

static void arg(Context* ctx, TreeNode* ast) {
  PRELUDE();
  if (ctx->regIndex < ARG_REGS_SIZE) {
    gen("POP ARG INTO REG",
        "\t\tpop %s\n",
        ARG_REGS[ctx->regIndex]);
    ctx->depth--;
    ctx->regIndex++;
  }
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
