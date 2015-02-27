#pragma once

#include "value.h"

struct environment;
typedef struct environment environment_t;

environment_t *env_new(environment_t *parent);
object_t *env_lookup(environment_t *env, const char *id);
void env_insert(environment_t *env, const char *id, object_t *val);
void env_update(environment_t *env, const char *id, object_t *val);
