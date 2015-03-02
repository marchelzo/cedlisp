#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/resource.h>
#include <unistd.h>

#include "eval.h"
#include "value.h"
#include "environment.h"

environment_t *global_env = NULL;

const char *error_message = NULL;

object_t **parse_program(const char *source); /* forward declaration */

char *initial_addr;
long long int max_stack_size;

static const char *keywords[] = {
  "fn",
  "seq"
};

object_t nil = {
  .type = CONS,
  .value = {
    .cons = {
      .car = NULL,
      .cdr = NULL
    }
  }
};

object_t boolean_true = {
  .type = ATOM,
  .value = {
    .atom = {
      .type = BOOLEAN,
      .value = {
        .boolean = true
      }
    }
  }
};

object_t boolean_false = {
  .type = ATOM,
  .value = {
    .atom = {
      .type = BOOLEAN,
      .value = {
        .boolean = false
      }
    }
  }
};


static bool is_keyword(const char *word)
{
  static size_t N = sizeof keywords / sizeof keywords[0];
  for (size_t i = 0; i < N; ++i)
    if (strcmp(word, keywords[i]) == 0) return true;
  return false;
}

result_t eval_keyword(const char *kw, object_t *args, environment_t *env);
result_t eval_function(object_t *fn, object_t *args, environment_t *env);

result_t eval(object_t *expr, environment_t *env)
{

  result_t result;

  char c;

  ptrdiff_t stack_size = initial_addr - &c;
  if (stack_size + 512 > max_stack_size) { ERROR("Maximum recursion depth exceeded"); }

  if (expr->type == ATOM) {
    if (expr->value.atom.type == IDENTIFIER) {
      object_t *val = env_lookup(env, expr->value.atom.value.identifier);
      if (!val) {
	ERROR("Reference to undefined variable");
      }
      OKAY(val);
    } else {
      OKAY(expr);
    }
  } else if (expr->type == CONS) {
    if (!expr->value.cons.car) {
      OKAY(&nil);
    }


    /* if it's an identifier, it could be a keyword */
    /* so we need to check for that rather than blindly evaluate */
    if (expr->value.cons.car->type == ATOM &&
	expr->value.cons.car->value.atom.type == IDENTIFIER &&
	is_keyword(expr->value.cons.car->value.atom.value.identifier)) {
      return eval_keyword(expr->value.cons.car->value.atom.value.identifier, expr->value.cons.cdr, env);
    }

    result_t fn = eval(expr->value.cons.car, env);

    if (fn.type == ERR) return fn;

    if (fn.result.value->type != ATOM || fn.result.value->value.atom.type != FUNCTION) {
      ERROR("Attempt to call non-function as a function");
    }

    return eval_function(fn.result.value, expr->value.cons.cdr, env);
  } else {
    ERROR("Internal interpreter error: object of invalid type");
  }
}


result_t eval_keyword(const char *kw, object_t *args, environment_t *env)
{
  result_t result;

  
  if (*kw == 'f') {

    object_t *fn = malloc(sizeof *fn);
    if (!fn) { ERROR("Out of memory!"); }

    fn->type = ATOM;
    fn->value.atom.type = FUNCTION;
    fn->value.atom.value.function.argc = 0;
    fn->value.atom.value.function.builtin = NULL;

    if (args->type != CONS) {
      free(fn);
      ERROR("Missing argument list in anonymous function expression");
    }

    /* parse arguments list */
    object_t *arg = args->value.cons.car;

    while (1) {
      if (arg == &nil) break;
      if (arg->type != CONS) { free(fn); ERROR("Invalid syntax in argument list of anonymous function"); }
      if (arg->value.cons.car->type != ATOM || arg->value.cons.car->value.atom.type != IDENTIFIER) { free(fn); ERROR("Non-identifier in argument list of anonymous function expression"); }
      fn->value.atom.value.function.argv[fn->value.atom.value.function.argc++] = arg->value.cons.car->value.atom.value.identifier;
      arg = arg->value.cons.cdr;
    }

    if (!args->value.cons.cdr->value.cons.car) { free(fn); ERROR("Anonymous function missing body"); }
    else fn->value.atom.value.function.body = args->value.cons.cdr->value.cons.car;

    /* TODO: maybe this should be a deep copy */
    fn->value.atom.value.function.env = env;

    OKAY(fn);

  } else if (*kw == 's') {

    if (args == &nil) { ERROR("Empty seq construct") }

    result_t val;

    while (1) {
      if (args == &nil) break;
      val = eval(args->value.cons.car, env);
      if (val.type == ERR) return val;
      args = args->value.cons.cdr;
    }
    
    return val;
    
  }
  ERROR("Interal interpreter error: eval_keyword called with invalid keyword");
}

result_t eval_function(object_t *fn, object_t *args, environment_t *env)
{
  result_t result;



  if (fn->value.atom.value.function.builtin) {
    return fn->value.atom.value.function.builtin(args, env, global_env);
  }

  /* parse and evaluate the arguments to the function */
  size_t n = 0; /* number of arguments */
  size_t argc = fn->value.atom.value.function.argc; /* number of required arguments */

  environment_t *call_env = env_new(fn->value.atom.value.function.env);
  
  while (1) {
    if (args == &nil) break;
    if (n > argc) { ERROR("Too many arguments in function call"); }
    result_t arg = eval(args->value.cons.car, env);
    if (arg.type == ERR) { return arg; }
    env_update_or_insert_local(call_env, fn->value.atom.value.function.argv[n], arg.result.value);
    args = args->value.cons.cdr;
    n += 1;
  }

  if (n != argc) { ERROR("Too few arguments in function call"); }

  return eval(fn->value.atom.value.function.body, call_env);
}

void set_max_stack_usage(long long int bytes)
{
  max_stack_size = bytes;
}

bool init(long long int stack_size)
{
  global_env = env_new(NULL);
  if (!global_env) {
    error_message = "Failed to initialize global environment";
    return false;
  }

  insert_builtins(global_env);
  max_stack_size = stack_size;

  return true;
}

bool evaluate(const char *s)
{
  object_t **expressions = parse_program(s);
  if (!expressions) {
    error_message = "Bad syntax";
    return false;
  }

  while (*expressions) {
    result_t val = eval(*expressions, global_env);
    if (val.type == ERR) {
      error_message = val.result.error;
      return false;
    }
    expressions += 1;
  }
  
  return true;
}

result_t eval_program(object_t **program, long long int max_stack_usage)
{
  result_t result;

  global_env = env_new(NULL);

  if (!global_env) {
    ERROR("Failed to allocate an environment!");
  }

  insert_builtins(global_env);


  if (!*program || !program) {
    ERROR("Nothing to evaluate!")
  }

  char c;
  initial_addr = &c;
  max_stack_size = max_stack_usage;
  
  while (*program) {

    result = eval(*program, global_env);

    if (result.type == ERR) {
      return result;
    }
    
    program += 1;
  }

  return result;
}
