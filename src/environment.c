#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "environment.h"

struct environment {
  struct environment *parent;
  size_t size;
  size_t allocated;
  const char **keys;
  object_t **values;
};

bool grow_env(environment_t *env)
{
  env->allocated += 4;

  const char **new_keys = realloc(env->keys, sizeof *new_keys * env->allocated);
  object_t **new_values = realloc(env->values, sizeof *new_values * env->allocated);

  if (!new_keys || !new_values) return false;

  env->keys = new_keys;
  env->values = new_values;

  return true;
}

environment_t *env_new(environment_t *parent)
{
  environment_t *env = malloc(sizeof *env);
  env->parent = parent;
  env->size = 0;
  env->allocated = 0;
  env->keys = NULL;
  env->values = NULL;
  return env;
}

object_t *env_lookup(environment_t *env, const char *id)
{
  for (size_t i = 0; i < env->size; ++i)
    if (strcmp(env->keys[i], id) == 0)
      return env->values[i];
  return NULL;
}

void env_insert(environment_t *env, const char *id, object_t *val)
{
  if (env->size == env->allocated && !grow_env(env)) return;

  env->keys[env->size] = id;
  env->values[env->size] = val;

  env->size += 1;
}

void env_update(environment_t *env, const char *id, object_t *val)
{
  while (env) {
    for (size_t i = 0; i < env->size; ++i) {
      if (strcmp(env->keys[i], id) == 0) {
	env->values[i] = val;
	return;
      }
    }
    env = env->parent;
  }
}
