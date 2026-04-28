#include "frontend/lexer.h"
#include "ds/hashtable/hashtable.h"
#include "logger/logger.h"
#include "utils/utils.h"
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static HashTable KEYWORD_HT = (HashTable){};
static size_t KEYWORD_HT_REFCOUNT = 0;

static Error keywordInit();
static bool isIn(char c, const char* str);

static const char* const TOKEN_TYPES[] = {
  #define X(enm, str) \
    [enm] = str,

  TOKEN_TYPE_LIST()
  #undef X
};
const size_t TOKEN_TYPES_SIZE = sizer(TOKEN_TYPES);

Error lexerInit(Lexer* lexer, const char* filename, size_t initCap) {
  if (!lexer    ||
      !filename || 
      !initCap)
    return BadArgs;

  int fd = open(filename, O_RDONLY);
  if (fd < 0)
    return FailFileOpen;

  Error err = OK;
  if ((err = dynArrInit(&lexer->tokens, initCap, sizeof(Token), NULL)))
    return err;

  if ((err = mappedFileInit(fd, &lexer->mf)))
    return err;

  close(fd);

  lexer->pos        = 0;
  lexer->line       = lexer->mf.data + 1;
  lexer->lineStart  = lexer->mf.data + 1;

  if (!KEYWORD_HT_REFCOUNT &&
      (err = keywordInit())) {
    return err;
  }
  KEYWORD_HT_REFCOUNT++;
  return OK;
}

Lexer* lexerAlloc(const char* filename, size_t initCap, Error* status) {
  if (!filename ||
      !initCap)
    RETURN_WITH_STATUS(BadArgs, NULL);

  Lexer* lexer = (Lexer*)calloc(1, sizeof(Lexer));
  if (!lexer)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = lexerInit(lexer, filename, initCap))) {
    free(lexer);
    RETURN_WITH_STATUS(err, NULL);
  }

  return lexer;
}

#define EMIT(T, position, length, ...) \
  {                                    \
    newToken = (Token){                \
      __VA_ARGS__ __VA_OPT__(,)        \
      .type = T,                       \
      .pos = lexer->mf.data + position,\
      .len = length,                   \
      .line = lexer->line,             \
      .lineStart = lexer->lineStart,   \
    };                                 \
    dynArrAppend(tokens, &newToken);   \
  }

#define CONSUME_CHAR(T, ...)        \
 {                                  \
    EMIT(T, lexer->pos, 1           \
        __VA_OPT__(,) __VA_ARGS__); \
    lexer->pos++;                   \
 }

static const char* const RESERVED_SPECIAL_CHARACTERS = ";(){}";
static const char* const ROMAN_NUMERAL_CHARS = "IVXLC";
static const char ROMAN_NUMERAL_PREFIX = '0';

Error lexerAnalyze(Lexer* lexer) {
  Error err = OK;
  if ((err = lexerVerify(lexer)))
    return err;
  
  const char* buf = lexer->mf.data;
  size_t bufSize = lexer->mf.size;
  Tokens* tokens = &lexer->tokens;
  Token newToken = {};
  for (char c = buf[lexer->pos]; lexer->pos < bufSize; c = buf[lexer->pos]) {
    //skip whitespace
    if (isspace(c)) {
      if (c == '\n') {
        lexer->line++;
        lexer->lineStart = lexer->mf.data + lexer->pos;
      }
      lexer->pos++;
      continue;
    }
    
    // one-character long tokens
    switch (c) {
      case ';': CONSUME_CHAR(TOK_SEMIC);  continue;
      case '(': CONSUME_CHAR(TOK_LPAREN); continue;
      case ')': CONSUME_CHAR(TOK_RPAREN); continue;
      case '{': CONSUME_CHAR(TOK_LBRACE); continue;
      case '}': CONSUME_CHAR(TOK_RBRACE); continue;
      default: break;
    }

    // numeric literals
    if (c == ROMAN_NUMERAL_PREFIX) {
      uint64_t num = 0;
      size_t oldPos = lexer->pos;
      lexer->pos++;
      if (lexer->pos < bufSize &&
          isIn(c = buf[lexer->pos], ROMAN_NUMERAL_CHARS)) {
        uint64_t degree = 0;
        uint64_t newDegree = 0;
        uint64_t numBuf = 0;
        bool cont = true;
        while (cont && lexer->pos < bufSize) {
          c = buf[lexer->pos];
          switch (c) {
            case 'I': newDegree = 1;  break;
            case 'V': newDegree = 5;  break;
            case 'X': newDegree = 10; break;
            case 'L': newDegree = 50; break;
            case 'C': newDegree = 100; break;
            default:  cont = false; num += numBuf; continue;
          }

          if (degree && 
              degree != newDegree) {
            if (degree < newDegree) {
              num -= numBuf;
            } else {
              num += numBuf;
            }
            numBuf = 0;
          }

          degree = newDegree;
          numBuf += degree;

          lexer->pos++;
        }
      }
      EMIT(TOK_NUM_LIT, 
           oldPos,
           lexer->pos - oldPos,
           .value = num);
      continue;
    }

    // identifiers
    size_t oldPos = lexer->pos;
    lexer->pos++;
    while (lexer->pos < bufSize &&
           !isspace(c = buf[lexer->pos]) &&
           !isIn(c, RESERVED_SPECIAL_CHARACTERS)) {
      lexer->pos++;
    }

    err = OK;
    size_t len = lexer->pos - oldPos;
    StringView strView = (StringView){ 
      .data = lexer->mf.data + oldPos, 
      .size = len 
    };
    TokenType kwType = hashTableGet(&KEYWORD_HT, strView, &err);
    if (err == NotFound) {
      EMIT(TOK_IDENTIFIER, oldPos, len);
    } else {
      EMIT(kwType, oldPos, len);
    }
    continue;

    // should be unreachable
    logln(ERROR, "Unknown character '%c'. Skipping...", c);
    lexer->pos++;
  }
  EMIT(TOK_EOF, lexer->pos, 0);
  return OK;
}

#undef EMIT
#undef CONSUME_CHAR

Error lexerDestroy(Lexer* lexer, bool isAlloced) {
  if (!lexer)
    return BadArgs;
  
  dynArrDestroy(&lexer->tokens, false);
  if (lexer->mf.data)
    mappedFileDestroy(&lexer->mf);
    
  if (isAlloced)
    free(lexer);

  KEYWORD_HT_REFCOUNT--;
  if (!KEYWORD_HT_REFCOUNT)
    hashTableDestroy(&KEYWORD_HT, false);
  return OK;
}

Error lexerPrintTokens(FILE* sink, Lexer* lexer) {
  if (!sink)
    return BadArgs;
  Error err = OK;
  if ((err = lexerVerify(lexer)))
    return err;

  for (size_t i = 0; i < lexer->tokens.count; i++) {
    Token* t = (Token*)dynArrGet(&lexer->tokens, i);
    const char* tStr = getTokenTypeStr(t->type);
    fprintf(sink, "%-2s", t->isInvalidClass ? "!" : "");
    switch (t->type) {
      case TOK_NUM_LIT:
        fprintf(sink, 
                "%s(%lu)(%.*s)\n", 
                tStr, t->value, 
                (int)t->len, t->pos);
        break;
      case TOK_IDENTIFIER:
        fprintf(sink, 
                "%s(%.*s)\n", 
                tStr, (int)t->len, t->pos);
        break;
      default:
        fprintf(sink, "%s\n", tStr);
    }
  }
  return OK;
}

Error lexerVerify(Lexer* lexer) {
  if (!lexer)
    return BadArgs;
  if (!lexer->mf.data)
    return NullPointerField;
  if (!lexer->mf.size)
    return ZeroSize;
  Error err = OK;
  if ((err = dynArrVerify(&lexer->tokens)))
    return err;
  return OK;
}

static const char* UNKNOWN_TOKEN_TYPE_STR = "UNKNOWN_TOKEN_TYPE";

const char* getTokenTypeStr(TokenType type) {
  if (type < 0 || type >= TOKEN_TYPES_SIZE)
    return UNKNOWN_TOKEN_TYPE_STR;

  return TOKEN_TYPES[type];
}

static bool isIn(char c, const char* str) {
  return strchr(str, c) != NULL;
}

static uint64_t hashdjb2(StringView strView);
static uint64_t hashRotate(StringView strView);

static const size_t KEYWORD_HT_BUCKET_SIZE = 101;
static const size_t KEYWORD_HT_LIST_CAPACITY = 4;
static const hash_f KEYWORD_HT_HASH_FUNC = hashRotate;

static Error keywordInit() {
  Error err = OK;
  if ((err = hashTableInit(&KEYWORD_HT, 
                           KEYWORD_HT_BUCKET_SIZE, 
                           KEYWORD_HT_LIST_CAPACITY, 
                           KEYWORD_HT_HASH_FUNC)))
    return err;

#define SV(str) (StringView){ .data = str, .size = sizeof(str) - 1 }
#define X(tok, str) if ((err = hashTablePut(&KEYWORD_HT, SV(str), tok))) return err;
  KEYWORD_LIST()
  KEYWORD_ALIAS_LIST()
#undef X
#undef SV

  return OK;
}

//NOTE: Mainly taken from: http://www.cse.yorku.ca/~oz/hash.html
static _unused uint64_t hashdjb2(StringView strView) {
  uint64_t hash = 5381;

  for (size_t i = 0; i < strView.size; i++)
    hash = ((hash << 5) + hash) + (uint64_t)strView.data[i]; /* hash * 33 + c */

  return hash;
}

static _unused uint64_t hashRotate(StringView strView) {
  uint64_t hash = 0;
 
  //NOTE: Source for left rotation: https://stackoverflow.com/a/13289498
  for (size_t i = 0; i < strView.size; i++) {
    hash = (hash << 1) | (hash >> ((sizeof(hash) - 1) % sizeof(hash)));
    hash ^= (uint64_t)strView.data[i];
  }

  return hash;
}
