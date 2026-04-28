#ifndef PREPARSER_H
#define PREPARSER_H

#include "error/error.h"
#include "frontend/lexer.h"

Error preparse(Tokens* tokens);

#endif
