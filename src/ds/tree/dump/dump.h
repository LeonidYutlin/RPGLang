#ifndef DUMP_H
#define DUMP_H

#include <stdio.h>
#include "ds/tree/tree.h"
#include "diff/context.h"

void treeDump(FILE* html, Variables* vars, TreeRoot* root,
              const char* commentary, const char* filename, int line);
void nodeDump(FILE* html, Variables* vars, TreeNode* node, 
              const char* commentary, const char* filename, int line);

#define treeDump(file, vars, root, commentary) \
        treeDump(file, vars, root, commentary, __FILE__, __LINE__)
#define nodeDump(file, vars, node, commentary) \
        nodeDump(file, vars, node, commentary, __FILE__, __LINE__)

FILE* openHtmlLogFile();
void  closeHtmlLogFile(FILE* html);

#endif
