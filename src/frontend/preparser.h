#ifndef PREPARSER_H
#define PREPARSER_H

#include "error/error.h"
#include "frontend/lexer.h"

bool preparse(Tokens* ts, Error* status);

#endif
