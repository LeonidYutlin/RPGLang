#include "backend/codegen.h"
#include <assert.h>
#include <stdarg.h>

// each register is 8 bytes
static const size_t REG_SIZE = 8;
static const char* const ARG_REGS[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};
static const size_t ARG_REGS_SIZE = sizer(ARG_REGS);

static void codegenRec(FILE* sink, TreeNode* ast, uint64_t* depth, int regNum);
static void gen_(FILE* sink, const char* commentary, 
                 const char* fmt, ...) _format(printf, 3, 4);

#define gen(commentary, fmt, ...) \
  gen_(sink, "; -- " commentary " --\n", fmt __VA_OPT__(,) __VA_ARGS__)
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
      "extern out\n"
      "section .text\n"
      "_start:\n");

  //TreeNode* mainBody = ast->left->right->left;
  codegenRec(sink, ast, NULL, -1);
  gen("EXIT",
      "\t\tmov rax, 0x3c ; syscall exit\n"
      "\t\tmov rdi, 123\n"
      "\t\tsyscall\n");
}

static void codegenRec(FILE* sink, TreeNode* ast, uint64_t* depth, int regNum) {
  if (!sink || !ast)
    return;

  uint64_t stmtDepth = 0;
  int regNumRight = regNum;
  if (OF_CTRL(ast, CTRL_SEMIC)) {
    depth = &stmtDepth;
    if (regNum >= 0)
      regNumRight++;
  } else if (OF_CTRL(ast, CTRL_FUNC_CALL)) {
    regNum = regNumRight = 0;
  }
  codegenRec(sink, ast->left, depth, regNum);
  codegenRec(sink, ast->right, depth, regNumRight);
  if (IS_NUM(ast)) {
    gen("PUSH",
        "\t\tpush %ld\n", 
        ast->data.value.num);
    if (depth)
      (*depth)++;
  } else if (IS_OP(ast)) {
    switch (ast->data.value.op) {
      case OP_ADD:
        gen("ADD",
            "\t\tpop rbx\n"
            "\t\tpop rax\n"
            "\t\tadd rax, rbx\n"
            "\t\tpush rax\n");
        if (depth)
         (*depth)--;
        break;
      case OP_SUB:
        gen("SUB",
            "\t\tpop rbx\n"
            "\t\tpop rax\n"
            "\t\tsub rax, rbx\n"
            "\t\tpush rax\n");
        if (depth)
         (*depth)--;
        break;
      default: break;
    }
  } else if (OF_CTRL(ast, CTRL_SEMIC)) {
    //printf("Hello i am a semicolone and my depth is %lu\n", stmtDepth);
    if (regNum >= 0 && regNum < (int)ARG_REGS_SIZE) {
      gen("POP ARG INTO REG",
          "\t\tpop %s\n",
          ARG_REGS[regNum]);
      stmtDepth--;
    }
    while (stmtDepth--)
      gen("POP UNUSED",
          "\t\tadd rsp, %zu\n",
          REG_SIZE);
  } else if (OF_CTRL(ast, CTRL_FUNC_CALL)) {
    StringView funcName = ast->left->data.value.id;
    gen("CALL",
        "\t\tcall %.*s\n",
        (int)funcName.size, funcName.data);
  }
}

#undef gen
#undef com

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
