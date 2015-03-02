#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>

#include "src/ced.h"

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

  ced_init(1000000);

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

      if (!ced_eval(lib)) {
        printf("Error reading file: %s", ced_error());
        break;
      }
      continue;
    }

    snprintf(expr, 4095, "(print %s \"\\n\")", line);

    if (!ced_eval(expr))
      printf("Error: %s\n", ced_error());

    add_history(line);
  }

  return 0;

err:
  fputs("Aborting...\n", stderr);
  return 1;
}
