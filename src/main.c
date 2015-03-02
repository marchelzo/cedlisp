#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "parser.h"
#include "value.h"
#include "eval.h"

#define MAX_SOURCE_LEN (1024 * 20)

int main(int argc, char *argv[])
{

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  FILE *f = fopen(argv[1], "r");

  if (!f) {
    fprintf(stderr, "%s: Failed to open %s: %s\n", argv[0], argv[1], strerror(errno));
    return EXIT_FAILURE;
  }

  char program_source[MAX_SOURCE_LEN + 1];
  int n = fread((void *)program_source, 1, MAX_SOURCE_LEN, f);
  program_source[n] = 0;
  fclose(f);

  object_t **program = parse_program(program_source);

  struct rlimit stack_size;

  getrlimit(RLIMIT_STACK, &stack_size);
  
  stack_size.rlim_cur = stack_size.rlim_max;

  setrlimit(RLIMIT_STACK, &stack_size);


  result_t result = eval_program(program, stack_size.rlim_max);

  if (result.type == ERR)
    puts(result.result.error);

  return 0;
}
