#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "eval.h"
#include "value.h"
#include "environment.h"

#define ERROR(msg) result.type = ERR; \
                   result.result.error = msg; \
		   return result;

#define OKAY(val) result.type = OK; \
                  result.result.value = (val); \
		  return result;

const char *keywords[] = {
  "fn",
  "if",
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


bool is_keyword(const char *word)
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
      fn->value.atom.value.function.argv[fn->value.atom.value.function.argc++] = arg->value.atom.value.identifier;
      arg = arg->value.cons.cdr;
    }

    if (!args->value.cons.cdr->value.cons.car) { free(fn); ERROR("Anonymous function missing body"); }
    else fn->value.atom.value.function.body = args->value.cons.cdr->value.cons.car;

    /* TODO: maybe this should be a deep copy */
    fn->value.atom.value.function.env = env;

    OKAY(fn);

  } else if (*kw == 'i') {
    ERROR("NOT IMPLEMENTED");
  } else if (*kw == 's') {
    ERROR("NOT IMPLEMENTED");
  }
  ERROR("Interal interpreter error: eval_keyword called with invalid keyword");
}

result_t eval_function(object_t *fn, object_t *args, environment_t *env)
{
  result_t result;
  
  /* parse and evaluate the arguments to the function */
  size_t n = 0; /* number of arguments */
  size_t argc = fn->value.atom.value.function.argc; /* number of required arguments */
  while (1) {
    if (args == &nil) break;
    if (n > argc) { ERROR("Too many arguments in function call"); }
    result_t arg = eval(args->value.cons.car, env);
    if (arg.type == ERR) { return arg; }
    env_insert(fn->value.atom.value.function.env, fn->value.atom.value.function.argv[n], arg.result.value);
    args = args->value.cons.cdr;
    n += 1;
  }

  if (n != argc) { ERROR("Too few arguments in function call"); }

  return eval(fn->value.atom.value.function.body, fn->value.atom.value.function.env);
}


result_t eval_program(object_t **program)
{
  result_t result;

  environment_t *env = env_new(NULL);

  if (!env) {
    ERROR("Failed to allocate an environment!");
  }
  

  if (!*program || !program) {
    ERROR("Nothing to evaluate!")
  }
  
  while (*program) {

    result = eval(*program, env);

    if (result.type == ERR) {
      return result;
    }
    
    *program += 1;
  }

  return result;
}
