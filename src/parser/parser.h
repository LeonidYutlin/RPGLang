#ifndef PARSER_H
#define PARSER_H

#include "ds/tree/node/node.h"
#include "lexer/lexer.h"

//TODO: do something with buf: 
//1. Store absolute pointers in tokens. 
//2. Store relative pointers in AST, but carry buf with AST's Root
//3. Store buf in Parser, but transform relative pointers to absolute pointers in AST
TreeNode* parse(Tokens* tokens, char* buf); 

#endif
