#pragma once

#include "value.h"

typedef struct {
  enum {
    OK,
    ERR
  } type;
  union {
    char *error;
    object_t *value;
  } result;
} result_t;

result_t eval_program(object_t **program);
