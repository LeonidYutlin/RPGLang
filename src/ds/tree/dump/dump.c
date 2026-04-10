#include "ds/tree/dump/dump.h"
#include "logger/logger.h"
#undef rootDump
#undef nodeDump

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "utils/utils.h"
#include "ds/queue/queue.h"

//these aren't static consts but macros because of how C treats
//constant-variable sized arrays (for some reason mistakes them for VLAs)
#define DOT_PATH_BUF_SZ  128
#define HTML_PATH_BUF_SZ 128
#define IMG_PATH_BUF_SZ  128
#define DOT_CMD_BUF_SZ   512

static const char* BG_COLOR      = "#FFFFFF";
static const char* BAD_OUTLINE   = "#602222";
static const char* BAD_FILL      = "#F02222";
static const char* DEFAULT_CELL  = "#F02222";
static const char* OP_CELL       = "#98D26B";
static const char* NUM_CELL      = "#7CA0CE";
static const char* VAR_CELL      = "#8673BA";
static const char* TABLE_OUTLINE = "#101510";
static const char* TABLE_FILL    = "#10151034";
static const char* ADDRESS_FILL  = "#10151034";
static const char* LEFT_FILL     = "#10151034";
static const char* RIGHT_FILL    = "#10151034";
static const char* PARENT_FILL   = "#10151034";
static const char* VALUE_FILL    = "#10151034";
static const char* TYPE_FILL     = "#10151034";
static const char* OK_EDGE       = "#2222E0";
static const char* BAD_EDGE      = "#E02222";
static const char* ROOT_OUTLINE  = "#666666";
static const char* ROOT_FILL     = "#DDDDDD";

static Error rootTextDump(FILE* f, TreeRoot* root,
                          const char* commentary, const char* filename, int line,
                          uint callCount);
static Error rootGraphDump(FILE* f,  Variables* vars, TreeRoot* root, uint callCount);
static void populateDot(FILE* dot, Variables* vars, TreeNode* node);
static void declareNode(FILE* dot, Variables* vars, TreeNode* node, bool bondFailed);
static void declareRank(FILE* dot, TreeNode* node, Queue** queue);
static void executeDot(FILE* f, uint callCount, char* dotPath);

#define WARNING_PREFIX(condition) (condition) ? "<b><body><font color=\"red\">[!]</font></body></b>" : ""

void rootDump(FILE* f, Variables* vars, TreeRoot* root, 
              const char* commentary, const char* filename, int line) {
  assert(f);
  assert(filename);
  assert(commentary);

  static uint callCount = 0;
  ++callCount;
  logln(INFO, "rootDump #%u started", callCount);

  if (rootTextDump(f, root, commentary, filename, line, callCount))
    return;

  rootGraphDump(f, vars, root, callCount);
  logln(INFO, "rootDump #%u ended", callCount);
}

#define DOT_HEADER_INIT(file)                                                               \
  fprintf(file,                                                                             \
          "digraph G {\n"                                                                   \
          "rankdir=TB;\n"                                                                   \
          "graph [bgcolor=\"%s\", pad=0.25, nodesep=0.55, "                                 \
                 "ranksep=0.9, splines=ortho, ordering=\"in\"];\n"                          \
          "node [shape=hexagon, style=\"filled\", color=\"%s\", penwidth=1.4, "             \
                "fillcolor=\"%s\", fontname=\"monospace\", fontsize=30];\n"                 \
          "edge [color=\"%s\", penwidth=2.5, weight = 0, arrowsize=0.8, arrowhead=vee];\n", \
          BG_COLOR,                                                                         \
          BAD_OUTLINE, BAD_FILL,                                                            \
          BG_COLOR);

void nodeDump(FILE* f, Variables* vars, TreeNode* node, 
              const char* commentary, const char* filename, int line) {
  assert(f);
  assert(filename);
  assert(commentary);

  static uint callCount = 0;
  ++callCount;

  logln(INFO, "nodeDump #%u started", callCount);

  if (!node) {
    fprintf(f,
            "%s\n"
            "Textual Dump:\n"
            "TreeNode Dump #%u called from %s:%d\n"
            "TreeNode [NULL] {}\n",
            commentary,
            callCount, filename, line);
    return;
  }
  fprintf(f,
          "%s\n"
          "TreeNode Dump #%u called from %s:%d\n",
          commentary,
          callCount, filename, line);
  char dotPath[DOT_PATH_BUF_SZ] = {};
  if (snTimestampedFilename(dotPath, DOT_PATH_BUF_SZ,
                  ".log/dot-", ".txt", callCount)) {
    loglnTraced(ERROR, "Dot file name composition failed for graph dump");
    return;
  }
  FILE* dot = fopen(dotPath, "w");
  if (!dot) {
    loglnTraced(ERROR, "Dot file open failed for this graph dump");
    return;
  }
  DOT_HEADER_INIT(dot);
  populateDot(dot, vars, node);
  fputs("}\n", dot);
  fclose(dot);
  fputs("Graphical Dump:\n", f);
  executeDot(f, callCount, dotPath);

  logln(INFO, "nodeDump #%u ended", callCount);
}

FILE* openHtmlLogFile(const char* path) {
  if (!path)
    return NULL;

  char name[HTML_PATH_BUF_SZ] = {}; 
  if (snTimestampedFilename(name, HTML_PATH_BUF_SZ,
                  path, ".html", 0))
    return NULL;

  FILE* f = fopen(name, "w");
  if (!f)
    return NULL;

  fprintf(f,
          "<pre><h1>%s</h1>\n",
          name);
  return f;
}

void closeHtmlLogFile(FILE* f) {
  if (f)
    fclose(f);
} 

static Error rootTextDump(FILE* f, TreeRoot* root,
                          const char* commentary, const char* filename, int line,
                          uint callCount) {
  if (!root) {
    fprintf(f,
            "%s\n"
            "Textual Dump:\n"
            "TreeRoot Dump #%u called from %s:%d\n"
            "TreeRoot [NULL] {}\n",
            commentary,
            callCount, filename, line);
    return Fail;
  }

  fprintf(f,
          "%s\n"
          "TreeRoot Dump #%u called from %s:%d\n"
          "Textual Dump:\n"
          "TreeRoot [%p] {\n"
          "\tnodeCount = %lu\n"
          "\t%srootNode = %p\n"
          "}\n",
          commentary,
          callCount, filename, line,
          root,
          root->nodeCount,
          WARNING_PREFIX(!root->rootNode), root->rootNode);

  return !root->rootNode ? Fail : OK;
}

static Error rootGraphDump(FILE* f, Variables* vars, TreeRoot* root, uint callCount) {
  assert(f);
  assert(!varsVerify(vars));
  assert(root);

  char dotPath[DOT_PATH_BUF_SZ] = {};
  if (snTimestampedFilename(dotPath, DOT_PATH_BUF_SZ,
                  ".log/dot-", ".txt", callCount)) {
    loglnTraced(ERROR, "Dot file name composition failed for graph dump");
    return Fail;
  }

  FILE* dot = fopen(dotPath, "w");
  if (!dot) {
    loglnTraced(ERROR, "Dot file open failed for graph dump");
    return Fail;
  }

  DOT_HEADER_INIT(dot);

  fprintf(dot,
          "root [shape=box, style=\"filled\", color=\"%s\", "
          "fillcolor=\"%s\", fontsize=14, label="
          "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\" color=\"%s\">"
          "<tr>"
              "<td bgcolor=\"%s\"><b>ROOT</b></td>"
          "</tr>"
          "<tr>"
          "<td bgcolor=\"%s\"><b>nodeCount:</b> %lu</td>"
          "</tr>"
          "<tr>"
              "<td bgcolor=\"%s\"><b>address:</b> %p</td>"
          "</tr>"
          "<tr>"
              "<td bgcolor=\"%s\"><b>rootNode:</b> %p</td>"
          "</tr>"
          "</table>"
          ">];\n",
          ROOT_OUTLINE, ROOT_FILL,
          TABLE_OUTLINE,
          TABLE_FILL,
          TABLE_FILL, root->nodeCount,
          TABLE_FILL, root,
          TABLE_FILL, root->rootNode);

  populateDot(dot, vars, root->rootNode);

  fprintf(dot, "root -> node%p [color=\"%s\"]\n", root->rootNode, OK_EDGE);
  fputs("}\n", dot);
  fclose(dot);

  fputs("Graphical Dump:\n", f);
  executeDot(f, callCount, dotPath);

  return OK;
}

#undef DOT_HEADER_INIT

static void populateDot(FILE* dot, Variables* vars, TreeNode* node) {
  assert(dot);
  assert(!varsVerify(vars));
  assert(node);

  declareNode(dot, vars, node, false);
  Queue* queue = NULL;
  declareRank(dot, node, &queue);
}

#define DECLARE_CHILD_NODE(child)                                                 \
   {                                                                              \
   if (child) {                                                                   \
     if (child->parent == node) {                                                 \
       fprintf(dot, "node%p -> node%p [color=\"%s\", arrowtail=vee, dir=both]\n", \
               node, child, OK_EDGE);                                             \
       declareNode(dot, vars, child, false);                                      \
     } else {                                                                     \
       fprintf(dot, "node%p -> node%p [color=\"%s\"]\n", node, child, BAD_EDGE);  \
       declareNode(dot, vars, child, true);                                       \
     }                                                                            \
   }                                                                              \
   }

static void declareNode(FILE* dot, Variables* vars, TreeNode* node, bool bondFailed) {
  assert(dot);
  assert(!varsVerify(vars));
  if (!node)
    return;
  
  const NodeTypeInfo* nodeInfo = parseNodeType(node->data.type);
  fprintf(dot,
          "node%p"
          "[shape=box, style=\"rounded, filled\", color=\"%s\", fillcolor=\"%s\", penwidth=2.1, fontsize=14, label="
          "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\" color=\"%s\">"
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>parent:</b> %p</td>"
          "</tr>"
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>type:</b> %s</td>"
          "</tr>",
          node,
          TABLE_OUTLINE,
          IS_OP(node)  ? OP_CELL  :
          IS_NUM(node) ? NUM_CELL :
          IS_VAR(node) ? VAR_CELL :
          DEFAULT_CELL,
          TABLE_OUTLINE,
          PARENT_FILL,  node->parent,
          nodeInfo ? TYPE_FILL     : BAD_FILL,  
          nodeInfo ? nodeInfo->str : "ERROR: no info for such NodeType");
  switch(node->data.type) {
    case OP_TYPE:
      {
        const OpTypeInfo* opInfo = parseOpType(node->data.value.op);
        fprintf(dot,
                "<tr>"
                  "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %s</td>"
                "</tr>",
                opInfo ? VALUE_FILL  : BAD_FILL, 
                opInfo ? opInfo->str : "ERROR: no info for such OpType");
      }
      break;
    case VAR_TYPE:
      {
        Variable* v = getVar(vars, node->data.value.var, NULL);
        fprintf(dot,
                "<tr>"
                  "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %s</td>"
                "</tr>",
                v ? VALUE_FILL : BAD_FILL, 
                v ? v->str     : "ERROR: no var with such index");
      }
      break;
    case NUM_TYPE:
      fprintf(dot,
              "<tr>"
                "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %lf</td>"
              "</tr>",
              VALUE_FILL, node->data.value.num);
      break;
    default:
      break;
  }
  fprintf(dot,
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>address:</b> %p</td>"
          "</tr>"
          "<tr>"
              "<td bgcolor=\"%s\"><b>left:</b> %p</td>"
              "<td bgcolor=\"%s\"><b>right:</b> %p</td>"
          "</tr>"
          "</table>"
          ">];\n",
          ADDRESS_FILL, node,
          node->data.type != OP_TYPE && node->left
          ? DEFAULT_CELL
          : LEFT_FILL, node->left,
          node->data.type != OP_TYPE && node->right
          ? DEFAULT_CELL
          : RIGHT_FILL, node->right);
  if (node->parent && bondFailed)
    fprintf(dot, "node%p -> node%p [color=\"%s\"]\n", node, node->parent, BAD_EDGE);
  DECLARE_CHILD_NODE(node->left);
  DECLARE_CHILD_NODE(node->right);
}

#undef DECLARE_CHILD_NODE

static void declareRank(FILE* dot, TreeNode* node, Queue** queue) {
  assert(dot);
  assert(node);
  assert(queue);

  fputs("{ rank = same; ", dot);
  Queue* newQueue = NULL;
  TreeNode* cur = node;
  do {
    fprintf(dot, "node%p; ", cur);
    if (cur->left)
      enqueue(&newQueue, cur->left);
    if (cur->right)
      enqueue(&newQueue, cur->right);
  } while (!dequeue(queue, &cur));
  fputs("}\n", dot);
  if (!dequeue(&newQueue, &cur))
    declareRank(dot, cur, &newQueue);
}

static void executeDot(FILE* f, uint callCount, char* dotPath) {
  char cmd[DOT_CMD_BUF_SZ] = {};
  char imgPath[IMG_PATH_BUF_SZ] = {};
  if (snTimestampedFilename(imgPath, IMG_PATH_BUF_SZ,
                  ".log/graph-", 
                  ".svg", callCount)) {
    loglnTraced(ERROR, "Image file path composition failed for graph dump!");
    return;
  }
  snprintf(cmd, DOT_CMD_BUF_SZ, "dot -T svg \"%s\" -o \"%s\"", dotPath, imgPath);
  system(cmd);

  fprintf(f, "<img src=\"./%s\"></img>\n", imgPath + strlen(".log/"));
}

#undef WARNING_PREFIX
