#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "parser.h"
#include "value.h"

static bool parse_string(char **, const char **);
static object_t *parse_object(const char **);
static object_t *parse_list(const char **);

bool parse_string(char **dst, const char **src)
{
  size_t bytes_allocated = 1;
  size_t len = 0;
  size_t offset = 0;

  *dst = malloc(bytes_allocated);
  if (!*dst) return NULL;
  
  char c;
  while ((*src)[len+offset] && (*src)[len+offset] != '"') {

    if ((*src)[len+offset] == '\\') {
      offset += 1;
      if (!(*src)[len+offset]) goto err;
      else if ((*src)[len+offset] == 'n') c = '\n';
      else if ((*src)[len+offset] == 't') c = '\t';
      else if ((*src)[len+offset] == '\\') c = '\\';
      else goto err;
    } else {
      c = (*src)[len+offset];
    }

    (*dst)[len++] = c;
    if (len == bytes_allocated) {
      bytes_allocated *= 2;
      char *new_dst = realloc(*dst, bytes_allocated);
      if (!new_dst) goto err;
      *dst = new_dst;
    }
  }

  (*dst)[len] = 0;
  *src += len + offset;
  return true;
err:
  free(*dst);
  *dst = NULL;
  return false;
}

object_t *parse_list(const char **source)
{
  object_t *list = malloc(sizeof *list);
  if (!list) return NULL;
  
  list->type = CONS;
  list->value.cons.car = parse_object(source);
  if (!list->value.cons.car) {
    free(list);
    return &nil;
  }
  if (**source == ' ') {
    *source += 1;
    list->value.cons.cdr = parse_list(source);
  } else {
    list->value.cons.cdr = &nil;
  }
  return list;
}

object_t *parse_object(const char **source)
{
  object_t *obj = malloc(sizeof *obj);
  if (!obj) return NULL;
  
  if (**source == '(') {
    *source += 1;
    obj->type = CONS;
    obj->value.cons.car = parse_object(source);
    if (!obj->value.cons.car) goto err;
    if (**source == ' ') {
      *source += 1;
      obj->value.cons.cdr = parse_list(source);
    } else {
      obj->value.cons.cdr = NULL;
    }
    if (**source != ')') goto err;
    *source += 1;
    return obj;
  } else {
    obj->type = ATOM;
    switch (**source) {
      case '"': {
	*source += 1;
	obj->value.atom.type = STRING;
	if (parse_string(&obj->value.atom.value.string, source)) {
	  break;
	} else {
	  goto err;
	}
	if (**source != '"') goto err;
	*source += 1;
      }
      case '\'': {
	*source += 1;
	obj->value.atom.type = QUOTED;
	obj->value.atom.value.quoted = parse_object(source);
	if (!obj->value.atom.value.quoted) goto err;
	return obj;
      }
      default: {
	if (isdigit(**source) || (**source == '-' && isdigit(source[0][1]))) {
	  char *resume;
	  long long k = strtoll(*source, &resume, 10);
	  if (k == 0 && errno == EINVAL) goto err;

	  if (*resume == '.') {
	    float f = strtof(*source, &resume);
	    if (f == 0 && resume == *source) goto err;
	    *source = resume;
	    obj->value.atom.type = REAL;
	    obj->value.atom.value.real = f;
	    return obj;
	  } else {
	    *source = resume;
	    obj->value.atom.type = INTEGER;
	    obj->value.atom.value.integer = k;
	    return obj;
	  }
	} else {
	  obj->value.atom.type = IDENTIFIER;
	  const char *s = *source;
	  while (*s && *s != ' ' && *s != '(' && *s != ')') ++s;
	  if (s == *source) goto err;
	  size_t len = s - *source;
	  obj->value.atom.value.identifier = malloc(len + 1);
	  if (!obj->value.atom.value.identifier) goto err;
	  for (size_t i = 0; *source != s; ++i) obj->value.atom.value.identifier[i] = *((*source)++);
	  obj->value.atom.value.identifier[len] = 0;
	  return obj;
	}
      }
    }
  }
err:
  free(obj);
  return NULL;
}

object_t **parse_program(const char *source)
{
  size_t n = 0;
  size_t objects_allocated = 1;
  object_t **program = malloc(sizeof *program);
  if (!program) return NULL;

  while (*source) {
    program[n++] = parse_object(&source);

    if (n == objects_allocated) {
      objects_allocated *= 2;
      object_t **new_program = realloc(program, sizeof *program * (objects_allocated));

      if (!new_program) {
	free(program);
	return NULL;
      } else {
	program = new_program;
      }
    }
    
  }

  program[n] = NULL;
  return program;
}
