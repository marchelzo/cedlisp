#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include "builtins.h"
#include "value.h"
#include "environment.h"
#include "eval.h"

#define BINOP(op)   result_t result; \
                    if (!assert_argc(2, args)) { \
                      ERROR("Invalid number of args to " #op) \
                    } else { \
                      result_t a = eval(args->value.cons.car, env); \
		      result_t b = eval(args->value.cons.cdr->value.cons.car, env); \
		      if (a.type == ERR) return a; \
		      if (b.type == ERR) return b; \
		      char same = same_numeric_type(a.result.value, b.result.value); \
                      if (!same) { ERROR("Invalid args to " #op) } \
		      if (same == 1) return new_integer(a.result.value->value.atom.value.integer op b.result.value->value.atom.value.integer); \
		      return new_real(a.result.value->value.atom.value.real op b.result.value->value.atom.value.real); \
		    }
    

result_t eval(object_t *, environment_t *);

static bool assert_argc(size_t n, object_t *argv)
{
  if (!n) return argv == &nil;

  if (argv->type != CONS || argv == &nil) return false;
  
  return assert_argc(n - 1, argv->value.cons.cdr);
}

static bool truthy(object_t *obj)
{
  if (obj->type == CONS)
    return obj != &nil;

  switch (obj->value.atom.type) {
  case FUNCTION: return true;
  case INTEGER: return obj->value.atom.value.integer;
  case REAL: return obj->value.atom.value.real;
  case STRING: return obj->value.atom.value.string[0];
  case BOOLEAN: return obj->value.atom.value.boolean;
  }
}

static bool same_type(object_t *a, object_t *b)
{
  if (a->type == CONS) return b->type == CONS;
  return b->type == ATOM && a->value.atom.type == b->value.atom.type;
}

static char same_numeric_type(object_t *a, object_t *b)
{
  if (a->type != ATOM || (a->value.atom.type != REAL && a->value.atom.type != INTEGER)) return false;
  if (a->value.atom.type == b->value.atom.type) {
    if (a->value.atom.type == INTEGER) return 1;
    return 2;
  }
  return 0;
}

static result_t new_integer(int64_t k)
{
  result_t result;
  object_t *integer = malloc(sizeof *integer);

  if (!integer) { ERROR("Out of memory!"); }

  integer->type = ATOM;
  integer->value.atom.type = INTEGER;
  integer->value.atom.value.integer = k;

  OKAY(integer);
}

static result_t new_real(float k)
{
  result_t result;
  object_t *real = malloc(sizeof *real);

  if (!real) { ERROR("Out of memory!"); }

  real->type = ATOM;
  real->value.atom.type = REAL;
  real->value.atom.value.real = k;

  OKAY(real);
}

static result_t new_boolean(bool b)
{
  result_t result;
  object_t *boolean = malloc(sizeof *boolean);

  if (!boolean) { ERROR("Out of memory!"); }

  boolean->type = ATOM;
  boolean->value.atom.type = BOOLEAN;
  boolean->value.atom.value.boolean = b;

  OKAY(boolean);
}

static void print_object(object_t *obj)
{
    if (obj->type == CONS) {
      putchar('(');
      putchar(')');
    } else {
      switch (obj->value.atom.type) {
      case FUNCTION:
	fputs("<#PROC#>", stdout);
	break;
      case STRING:
	fputs(obj->value.atom.value.string, stdout);
	break;
      case INTEGER:
	printf("%"PRIi64, obj->value.atom.value.integer);
	break;
      case REAL:
	printf("%f", obj->value.atom.value.real);
	break;
      case BOOLEAN:
	fputs(obj->value.atom.value.boolean ? "t" : "f", stdout);
	break;
      case QUOTED:
	fputs("<#CODE#>", stdout);
	break;
      }
    }
}

static result_t builtin_equal(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;
  
  if (!assert_argc(2, args)) { ERROR("Invalid # of args to ="); }
  
  result_t a_r = eval(args->value.cons.car, env);
  result_t b_r = eval(args->value.cons.cdr->value.cons.car, env);

  if (a_r.type == ERR) return a_r;
  if (b_r.type == ERR) return b_r;

  object_t *a = a_r.result.value;
  object_t *b = b_r.result.value;

  if (!same_type(a,b)) { ERROR("Arguments to = must have the same type"); }

  if (a->type == CONS) {

  } else {
    switch (a->value.atom.type) {
    case INTEGER: return new_boolean(a->value.atom.value.integer == b->value.atom.value.integer);
    }
  }
}

static result_t builtin_if(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (!assert_argc(3, args)) { ERROR("Invalid # of args to if"); }

  result_t cond = eval(args->value.cons.car, env);
  if (cond.type == ERR) return cond;
  
  if (truthy(cond.result.value)) {
    return eval(args->value.cons.cdr->value.cons.car, env);
  } else {
    return eval(args->value.cons.cdr->value.cons.cdr->value.cons.car, env);
  }
}

static result_t builtin_print(object_t *args, environment_t *env, environment_t *global)
{
  result_t val = eval(args->value.cons.car, env);

  if (val.type == ERR) {
    puts(val.result.error);
  } else {
    print_object(val.result.value);
  }

  if (args->value.cons.cdr != &nil)
    return builtin_print(args->value.cons.cdr, env, global);
  else
    return (result_t) { .type = OK, .result = { .value = &nil } };
}

static result_t builtin_add(object_t *args, environment_t *env, environment_t *global)
{
  BINOP(+)
}

static result_t builtin_subtract(object_t *args, environment_t *env, environment_t *global)
{
  BINOP(-)
}

static result_t builtin_multiply(object_t *args, environment_t *env, environment_t *global)
{
  BINOP(*)
}

static result_t builtin_divide(object_t *args, environment_t *env, environment_t *global)
{
  BINOP(/)
}

static result_t builtin_set_global(object_t *args, environment_t *env, environment_t *global)
{
  result_t val = eval(args->value.cons.cdr->value.cons.car, env);
  if (val.type == ERR) return val;
  env_insert(global, args->value.cons.car->value.atom.value.identifier, val.result.value);
}


/* builtin fuctions */
object_t _builtin_add        = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_add } } } } };
object_t _builtin_subtract   = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_subtract } } } } };
object_t _builtin_multiply   = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_multiply } } } } };
object_t _builtin_divide     = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_divide } } } } };
object_t _builtin_print      = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_print } } } } };
object_t _builtin_set_global = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_set_global } } } } };
object_t _builtin_if         = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_if } } } } };
object_t _builtin_equal      = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_equal } } } } };

void insert_builtins(struct environment *env)
{
  env_insert(env, "+", &_builtin_add);
  env_insert(env, "-", &_builtin_subtract);
  env_insert(env, "*", &_builtin_multiply);
  env_insert(env, "/", &_builtin_divide);
  env_insert(env, "print", &_builtin_print);
  env_insert(env, "set!", &_builtin_set_global);
  env_insert(env, "if", &_builtin_if);
  env_insert(env, "=", &_builtin_equal);
}
