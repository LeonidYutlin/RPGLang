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

static void codegenRec(Context* ctx, TreeNode* ast, 
                       bool isIfBody, uint64_t endLabel);
static inline void push(Context* ctx, TreeNode* ast);
static inline void op(Context* ctx, TreeNode* ast);
static inline void ctrl(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static inline void call(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static inline void arg(Context* ctx, TreeNode* ast);
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
    .regIndex = 0,
  };
  codegenRec(&ctx, ast, false, 0);
  gen("EXIT",
      "\t\tmov rax, 0x3c ; syscall exit\n"
      "\t\tmov rdi, 123\n"
      "\t\tsyscall\n");
}

#undef sinkFile
#define sinkFile ctx->sink

static void codegenRec(Context* ctx, TreeNode* ast, 
                       bool isIfBody, uint64_t endLabel) {
  if (!ctx || 
      !ctx->sink ||
      !ast)
    return;

  uint64_t oldDepth = ctx->depth;

  if (OF_CTRL(ast, CTRL_IF)) {
    endLabel = ctx->labelCount++;
    codegenRec(ctx, ast->left, false, 0);
    gen("IF",
        "\t\tpop rax\n"
        "\t\ttest rax, rax\n"
        "\t\tjz .if_end%zu\n",
        endLabel);
    ctx->depth--;
    codegenRec(ctx, ast->right, true, endLabel);
    return;
  }

  codegenRec(ctx, ast->left, false, 0);
  uint64_t rightEndLabel = 0;
  if (isIfBody) {
    if (OF_CTRL(ast->right, CTRL_ELSE)) {
      rightEndLabel = ctx->labelCount++;
      genn("\t\tjmp .else_end%zu\n", rightEndLabel);
    }
    genn(".if_end%zu:\n", endLabel);
  } else if (OF_CTRL(ast, CTRL_ELSE))
    genn(".else_end%zu:\n", endLabel);
  codegenRec(ctx, ast->right, false, rightEndLabel);


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
      // NOTE: i'm not pleased with this implementation
      // alternative could be cmovz rdx, 0xFFFFFFFF
      gen("DIV",
          "\t\txor edx, edx\n"
          "\t\ttest rax, 0x80000000\n"
          "\t\tjz .no_sign_extension%zu\n"
          "\t\tdec rdx\n"
          ".no_sign_extension%zu:\n"
          "\t\tidiv rbx\n"
          "\t\tpush rax\n",
          ctx->labelCount, ctx->labelCount);
      ctx->labelCount++;
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
    case OP_NOT:
      gen("NOT",
          "\t\tmov rcx, rax\n"
          "\t\txor eax, eax\n"
          "\t\ttest rcx, rcx\n"
          "\t\tmov rcx, 1\n"
          "\t\tcmovz rax, rcx\n"
          "\t\tpush rax\n");
      ctx->depth++;
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
    case OP_NOT:
      {
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
        ctx->depth++;
      }
      break;
#endif
    case OP_AND:
    {
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
    break;
    case OP_OR:
    {
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
    break;
    default: break;
  }
}

static void ctrl(Context* ctx, TreeNode* ast, uint64_t oldDepth) {
  PRELUDE();
  switch (ast->data.value.ctrl) {
    case CTRL_SEMIC: 
      clearStack(ctx, ast, oldDepth);
      break;
    case CTRL_ARG:
      arg(ctx, ast); 
      break;
    case CTRL_FUNC_CALL:
      call(ctx, ast, oldDepth);
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
