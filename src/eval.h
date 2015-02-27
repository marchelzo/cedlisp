#pragma once

#include "value.h"

#define ERROR(msg) result.type = ERR; \
                   result.result.error = msg; \
		   return result;

#define OKAY(val) result.type = OK; \
                  result.result.value = (val); \
		  return result;

typedef struct result {
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
