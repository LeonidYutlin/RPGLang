#ifndef PARSER_H
#define PARSER_H

#include "ds/tree/node/node.h"
#include "lexer/lexer.h"

TreeNode* parse(Tokens* tokens, char* buf); //TODO: new struct Parser that gets needed fields from lexer

#endif
