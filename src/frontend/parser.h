#ifndef PARSER_H
#define PARSER_H

#include "ds/tree/node/node.h"
#include "frontend/lexer.h"

TreeNode* parse(Tokens* tokens); 

#endif
