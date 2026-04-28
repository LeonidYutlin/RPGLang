#include "frontend/preparser.h"

typedef enum {
  None,
  Mage,
  Priest,
  Warrior,
  Warlock,
} Class;

//TODO: maybe another solution that doesnt involve Y-macro
#define CLASSIFIED_TOKENS_LIST() \
  X(TOK_EMPOWER, Mage)           \
  X(TOK_NUM_LIT, Mage)           \
  X(TOK_MIRROR, Mage)            \
  X(TOK_HIT, Warrior)            \
  X(TOK_DUELL, Warrior)          \
  X(TOK_DUELR, Warrior)          \
  X(TOK_DUELLR, Warrior)         \
  X(TOK_WORTHY, Warrior)         \
  X(TOK_PUSHL, Warrior)          \
  X(TOK_PUSHR, Warrior)          \
  X(TOK_SHADOW, Priest)          \
  X(TOK_WHILE, Priest)           \
  X(TOK_UNITE, Priest)           \
  X(TOK_AND, Priest)             \
  Y(TOK_NOT, Priest, Warlock)    \
  X(TOK_UNTIL, Warlock)          \
  X(TOK_SHATTER, Warlock)        \
  X(TOK_OR, Warlock)

static bool isInvalidClass(TokenType type, Class curClass);

Error preparse(Tokens* ts) {
  Error err = OK;
  if ((err = dynArrVerify(ts)))
    return err;

  Tokens newTs = (Tokens){};
  if ((err = dynArrInit(&newTs, ts->capacity, sizeof(Token), NULL)))
    return err;

  Class curClass = None;
  for (size_t i = 0; i < ts->count; i++) {
    Token* t = (Token*)dynArrGet(ts, i);
    switch (t->type) {
      case TOK_MAGE:    curClass = Mage;    continue;
      case TOK_PRIEST:  curClass = Priest;  continue;
      case TOK_WARRIOR: curClass = Warrior; continue;
      case TOK_WARLOCK: curClass = Warlock; continue;
      default: {
        t->isInvalidClass = isInvalidClass(t->type, curClass);
        dynArrAppend(&newTs, t); break;
      }
    }
  }

  dynArrDestroy(ts, false);
  *ts = newTs;

  return OK;
}

static bool isInvalidClass(TokenType type, Class curClass) {
#define X(tok, class)           case tok: return class != curClass;
#define Y(tok, class, altClass) case tok: return class != curClass && altClass != curClass;
  switch (type) {
    CLASSIFIED_TOKENS_LIST()
    default: return false;
  }
#undef X
#undef Y
}
