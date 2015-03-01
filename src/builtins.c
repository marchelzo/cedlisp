#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

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
  case QUOTED: return obj != &quoted_nil && obj->value.atom.value.quoted != &nil;
  case IDENTIFIER: return true;
  }
}

static result_t new_quoted(object_t *val)
{
  result_t result;

  object_t *quoted = malloc(sizeof *quoted);
  if (!quoted) { ERROR("Out of memory!"); }

  quoted->type = ATOM;
  quoted->value.atom.type = QUOTED;
  quoted->value.atom.value.quoted = val;

  OKAY(quoted);
}

static inline bool is_list(object_t *val)
{
  return val->type == ATOM && val->value.atom.type == QUOTED && val->value.atom.value.quoted->type == CONS;
}

static inline bool same_type(object_t *a, object_t *b)
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
  static result_t t = {
    .type = OK,
    .result = {
      .value = &boolean_true
    }
  };

  static result_t f = {
    .type = OK,
    .result = {
      .value = &boolean_false
    }
  };

  return b ? t : f;
}

static inline object_t *new_cons_cell(object_t *car, object_t *cdr)
{
  object_t *cons = malloc(sizeof *cons); /* the unquoted cons cell */
  if (!cons) return NULL;

  cons->type = CONS;
  cons->value.cons.car = car;
  cons->value.cons.cdr = cdr;

  return cons;
}

static result_t new_cons(object_t *car, object_t *cdr)
{
  result_t result;

  object_t *value = malloc(sizeof *value);
  if (!value) { ERROR("Out of memory!"); }

  value->type = ATOM;
  value->value.atom.type = QUOTED;

  object_t *cons = new_cons_cell(car, cdr);
  if (!value) { ERROR("Out of memory!"); }

  value->value.atom.value.quoted = cons;

  OKAY(value);
}

static void print_object(object_t *obj, bool inspect_quoted)
{
  if (obj->type == CONS) {
  
    putchar('(');
    while (obj != &nil && obj != &quoted_nil) {
      print_object(obj->value.cons.car, true);
      if (obj->value.cons.cdr != &nil && obj->value.cons.cdr != &quoted_nil)
  	putchar(' ');
      obj = obj->value.cons.cdr;
    }
    putchar(')');
  
  } else {
    switch (obj->value.atom.type) {
      case FUNCTION:
        fputs("<#PROC#>", stdout);
        break;
      case IDENTIFIER:
        fputs(obj->value.atom.value.identifier, stdout);
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
      case QUOTED: {
        if (inspect_quoted) {
	  print_object(obj->value.atom.value.quoted, false);
	} else {
          putchar('\'');
          print_object(obj->value.atom.value.quoted, false);
	}
	break;
      }
    }
  }
}

static result_t builtin_greater_than(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (!assert_argc(2, args)) { ERROR("Invlalid # of args to >"); }

  result_t a_r = eval(args->value.cons.car, env);
  result_t b_r = eval(args->value.cons.cdr->value.cons.car, env);

  if (a_r.type == ERR) return a_r;
  if (b_r.type == ERR) return b_r;

  char c = same_numeric_type(a_r.result.value, b_r.result.value);

  if (!c) { ERROR("Arguments to > must have the same numeric type"); }

  if (c == 1)
    return new_boolean(a_r.result.value->value.atom.value.integer > b_r.result.value->value.atom.value.integer);
  else
    return new_boolean(a_r.result.value->value.atom.value.real > b_r.result.value->value.atom.value.real);
}

static result_t builtin_less_than(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (!assert_argc(2, args)) { ERROR("Invlalid # of args to <"); }

  result_t a_r = eval(args->value.cons.car, env);
  result_t b_r = eval(args->value.cons.cdr->value.cons.car, env);

  if (a_r.type == ERR) return a_r;
  if (b_r.type == ERR) return b_r;

  char c = same_numeric_type(a_r.result.value, b_r.result.value);

  if (!c) { ERROR("Arguments to < must have the same numeric type"); }

  if (c == 1)
    return new_boolean(a_r.result.value->value.atom.value.integer < b_r.result.value->value.atom.value.integer);
  else
    return new_boolean(a_r.result.value->value.atom.value.real < b_r.result.value->value.atom.value.real);
}

static result_t builtin_eval(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (!assert_argc(1, args)) { ERROR("Invalid # of args to eval"); }
  
  result_t val = eval(args->value.cons.car, env);
  if (val.type == ERR) return val;

  if (val.result.value->type == ATOM && val.result.value->value.atom.type == QUOTED) {
    return eval(val.result.value->value.atom.value.quoted, env);
  }

  return val;
}

static result_t builtin_not(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (!assert_argc(1, args)) { ERROR("Invalid # of args to !"); }

  result_t val = eval(args->value.cons.car, env);

  if (val.type == ERR) return val;

  return new_boolean(!truthy(val.result.value));
}

/* usage: (list 1 2 "three" '4 (+ 0 5)) */
static result_t builtin_list(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (args == &nil) { OKAY(&nil); }

  object_t *head = malloc(sizeof *head);
  if (!head) { ERROR("Out of memory!"); }

  head->type = CONS;
  
  result_t arg = eval(args->value.cons.car, env);
  if (arg.type == ERR) return arg;
  
  head->value.cons.car = arg.result.value;
  head->value.cons.cdr = &nil;

  object_t *current = head;

  args = args->value.cons.cdr;

  while (args != &nil) {

    arg = eval(args->value.cons.car, env);
    if (arg.type == ERR) return arg;

    current->value.cons.cdr = malloc(sizeof (object_t));
    if (!current->value.cons.cdr) { ERROR("Out of memory!"); }

    current->value.cons.cdr->type = CONS;
    current->value.cons.cdr->value.cons.car = arg.result.value;
    current->value.cons.cdr->value.cons.cdr = &nil;

    current = current->value.cons.cdr;
    args = args->value.cons.cdr;
  }

  object_t *list = malloc(sizeof *list);
  if (!list) { ERROR("Out of memory!"); }

  list->type = ATOM;
  list->value.atom.type = QUOTED;
  list->value.atom.value.quoted = head;

  OKAY(list);
}

static result_t builtin_cons(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;
  
  if (!assert_argc(2, args)) { ERROR("Invalid # of args to cons"); }

  result_t a_r = eval(args->value.cons.car, env);
  result_t b_r = eval(args->value.cons.cdr->value.cons.car, env);

  if (a_r.type == ERR) return a_r;
  if (b_r.type == ERR) return b_r;

  if (b_r.result.value->type != ATOM ||
      b_r.result.value->value.atom.type != QUOTED ||
      b_r.result.value->value.atom.value.quoted->type != CONS) { ERROR("The second argument to cons must be a list"); }

  return new_cons(a_r.result.value, b_r.result.value->value.atom.value.quoted);
}

static result_t builtin_car(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (!assert_argc(1, args)) { ERROR("Invalid # of args to car"); }

  result_t val = eval(args->value.cons.car, env);

  if (val.type == ERR) return val;

  object_t *cons = val.result.value;

  if (!is_list(cons)) { ERROR("Non-list passed to car"); }

  if (cons == &quoted_nil) { OKAY(cons); }

  return eval(cons->value.atom.value.quoted->value.cons.car, env);
}

static result_t builtin_cdr(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  if (!assert_argc(1, args)) { ERROR("Invalid # of args to cdr"); }

  result_t val = eval(args->value.cons.car, env);

  if (val.type == ERR) return val;

  object_t *cons = val.result.value;

  if (!is_list(cons)) { ERROR("Non-list passed to car"); }

  if (cons == &quoted_nil) { OKAY(cons); }

  return new_quoted(cons->value.atom.value.quoted->value.cons.cdr);
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
    case STRING: return new_boolean(strcmp(a->value.atom.value.string, b->value.atom.value.string) == 0);
    case REAL: return new_boolean(a->value.atom.value.real == b->value.atom.value.real);
    default: new_boolean(a == b);
    }
  }
}

static result_t builtin_eq(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;
  
  if (!assert_argc(2, args)) { ERROR("Invalid # of args to ="); }
  
  result_t a_r = eval(args->value.cons.car, env);
  result_t b_r = eval(args->value.cons.cdr->value.cons.car, env);

  if (a_r.type == ERR) return a_r;
  if (b_r.type == ERR) return b_r;

  object_t *a = a_r.result.value;
  object_t *b = b_r.result.value;

  if (!same_type(a,b)) { return new_boolean(false); }

  if (a->type == CONS) {

  } else {
    switch (a->value.atom.type) {
    case INTEGER: return new_boolean(a->value.atom.value.integer == b->value.atom.value.integer);
    case STRING: return new_boolean(strcmp(a->value.atom.value.string, b->value.atom.value.string) == 0);
    case REAL: return new_boolean(a->value.atom.value.real == b->value.atom.value.real);
    default: return new_boolean(a == b);
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
    print_object(val.result.value, true);
  }

  if (args->value.cons.cdr != &nil)
    return builtin_print(args->value.cons.cdr, env, global);
  else
    return (result_t) { .type = OK, .result = { .value = &quoted_nil } };
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
  result_t result;

  result_t val = eval(args->value.cons.cdr->value.cons.car, env);
  if (val.type == ERR) return val;
  env_update_or_insert_local(global, args->value.cons.car->value.atom.value.identifier, val.result.value);
  OKAY(&quoted_nil);
}

static result_t builtin_set_local(object_t *args, environment_t *env, environment_t *global)
{
  result_t result;

  result_t val = eval(args->value.cons.cdr->value.cons.car, env);
  if (val.type == ERR) return val;
  env_update_or_insert_local(env, args->value.cons.car->value.atom.value.identifier, val.result.value);
  OKAY(&quoted_nil);
}


/* builtin fuctions */
object_t _builtin_not          = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_not } } } } };
object_t _builtin_add          = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_add } } } } };
object_t _builtin_subtract     = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_subtract } } } } };
object_t _builtin_multiply     = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_multiply } } } } };
object_t _builtin_divide       = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_divide } } } } };
object_t _builtin_print        = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_print } } } } };
object_t _builtin_set_global   = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_set_global } } } } };
object_t _builtin_set_local    = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_set_local } } } } };
object_t _builtin_if           = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_if } } } } };
object_t _builtin_equal        = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_equal } } } } };
object_t _builtin_eq           = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_eq } } } } };
object_t _builtin_greater_than = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_greater_than } } } } };
object_t _builtin_less_than    = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_less_than } } } } };
object_t _builtin_cons         = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_cons } } } } };
object_t _builtin_car          = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_car } } } } };
object_t _builtin_cdr          = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_cdr } } } } };
object_t _builtin_eval         = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_eval } } } } };
object_t _builtin_list         = { .type = ATOM, .value = { .atom = { .type = FUNCTION, .value = { .function = { .builtin = builtin_list } } } } };

void insert_builtins(struct environment *env)
{
  env_insert(env, "nil", &quoted_nil);
  env_insert(env, "!", &_builtin_not);
  env_insert(env, "+", &_builtin_add);
  env_insert(env, "-", &_builtin_subtract);
  env_insert(env, "*", &_builtin_multiply);
  env_insert(env, "/", &_builtin_divide);
  env_insert(env, "print", &_builtin_print);
  env_insert(env, "set!", &_builtin_set_global);
  env_insert(env, "set", &_builtin_set_local);
  env_insert(env, "if", &_builtin_if);
  env_insert(env, "=", &_builtin_equal);
  env_insert(env, "eq", &_builtin_eq);
  env_insert(env, ">", &_builtin_greater_than);
  env_insert(env, "<", &_builtin_less_than);
  env_insert(env, "cons", &_builtin_cons);
  env_insert(env, "car", &_builtin_car);
  env_insert(env, "cdr", &_builtin_cdr);
  env_insert(env, "eval", &_builtin_eval);
  env_insert(env, "list", &_builtin_list);
}
