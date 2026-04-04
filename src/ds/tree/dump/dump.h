#ifndef DUMP_H
#define DUMP_H

#include <stdio.h>
#include "ds/tree/root.h"
#include "diff/context.h"

void rootDump(FILE* html, Variables* vars, TreeRoot* root,
              const char* commentary, const char* filename, int line);
void nodeDump(FILE* html, Variables* vars, TreeNode* node, 
              const char* commentary, const char* filename, int line);

#define rootDump(file, vars, root, commentary) \
        rootDump(file, vars, root, commentary, __FILE__, __LINE__)
#define nodeDump(file, vars, node, commentary) \
        nodeDump(file, vars, node, commentary, __FILE__, __LINE__)

/// if the directory in the dirPath doesn't exist, will return NULL
FILE* openHtmlLogFile(const char* dirPath);
void  closeHtmlLogFile(FILE* html);

#endif
