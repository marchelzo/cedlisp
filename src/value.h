#pragma once

#define ARGC_MAX 12

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "builtins.h"

struct environment;
struct object;

typedef struct {
  uint8_t argc;
  char *argv[ARGC_MAX];
  builtin_function builtin;
  struct environment *env;
  struct object *body;
} function_t;

typedef struct object {
  enum {
    ATOM,
    CONS
  } type;
  union {
    struct {
      struct object *car;
      struct object *cdr;
    } cons;
    struct {
      enum {
	INTEGER,
	REAL,
	STRING,
	QUOTED,
	IDENTIFIER,
	FUNCTION,
	BOOLEAN
      } type;
      union {
	bool boolean;
	int64_t integer;
	float real;
	char *identifier;
	char *string;
	struct object *quoted;
	function_t function;
      } value;
    } atom;
  } value;
} object_t;

extern object_t nil;
extern object_t quoted_nil;
extern object_t boolean_true;
extern object_t boolean_false;
