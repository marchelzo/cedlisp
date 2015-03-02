#include "ced.h"
#include "eval.h"

bool ced_init(long long bytes)
{
  return init(bytes);
}

bool ced_eval(const char *code)
{
  return evaluate(code);
}

const char *ced_error(void)
{
  return error_message;
}
