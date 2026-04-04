#ifndef PARSE_H
#define PARSE_H

#include "diff/context.h"

//Dumps errors in stderr...
TreeNode* parseFormula(const char* expression, Variables* vars);

#endif
