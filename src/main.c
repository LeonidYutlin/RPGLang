#include "ds/dump.h"
#include "lexer/lexer.h"
#include "logger/logger.h"
#include "error/error.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
    return 1;
  }

  loggerInit(".log/!latest.txt", DEBUG);

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    logln(FATAL, "Failed to open");
    return 1;
  }

  Error err = OK;
  Lexer* lexer = lexerAlloc(fd, 16, &err);
  if (err) {
    logln(FATAL, "lexerAlloc returned %s", parseError(err)->str);
    return 1;
  }

  //FILE* logFile = openHtmlLogFile("./.log/");
  //if (!logFile)
  //  return 1;
  //hashTableDump(logFile, &KEYWORD_HT, "test");

  if ((err = lexerAnalyze(lexer))) {
    logln(FATAL, "lexerAnalyze returned %s", parseError(err)->str);
    return 1;
  }

  lexerPrintTokens(stdout, lexer); 

  lexerDestroy(lexer);
  close(fd);
  loggerCloseFile();
  return 0;
}


#define SV(str) (StringView){ .data = str, .size = sizeof(str) - 1 }

/*
int main() {
  loggerInit(".log/!latest.txt", DEBUG);
  FILE* logFile = openHtmlLogFile("./.log/");
  if (!logFile)
    return 80;
  Error err = OK;
  HashTable* t = hashTableAlloc(17, 8, hash, &err);
  if (err)
    return err;
  
  hashTableDump(logFile, t, "<h2>alloced</h2>");

#define X(tok, str) hashTablePut(t, SV(str), tok);
  KEYWORD_LIST()
#undef X

  hashTableDump(logFile, t, "<h2>Added elements</h2>");

  char buf[] = "Cat";

  uint64_t v = hashTableGet(t, SV("#warlock"), &err);
  if (err != OK &&
      err != NotFound)
    return err;
  printf("#warlock is %lu\n", v);

  v = hashTableGet(t, (StringView){.data = buf, .size = 3}, &err);
  if (err != OK &&
      err != NotFound)
    return err;
  printf("Cat is %lu\n", v);

  hashTableDestroy(t, true);
  fclose(logFile);
  loggerCloseFile();
  return 0;
}
*/
