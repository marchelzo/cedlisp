#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>

#include "src/parser.h"
#include "src/value.h"
#include "src/eval.h"
#include "src/environment.h"
#include "src/builtins.h"

void _Noreturn goodbye(int k)
{
  puts("\nGoodbye!");
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{

  signal(SIGINT, goodbye);

  static char *line = NULL;
  static char expr[4096];
  static char lib[8192];

  init(1000000);

  while (1) {

    line = readline("[λ] ==> ");

    if (!line) goto err;
    if (!*line) continue;

    if (*line == ':') {
      FILE *f = fopen(line + 1, "r");
      if (!f) {
        puts("Failed to open file");
        continue;
      }
      int n = fread((void *)lib, 1, 8191, f);
      lib[n] = 0;
      fclose(f);

      if (!evaluate(lib)) {
        printf("Error reading file: %s", error_message);
        break;
      }
      continue;
    }

    snprintf(expr, 4095, "(print %s \"\\n\")", line);

    if (!evaluate(expr))
      printf("Error: %s\n", error_message);

    add_history(line);
  }

  return 0;

err:
  fputs("Aborting...\n", stderr);
  return 1;
}
