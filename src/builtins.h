#pragma once

struct object;
struct environment;
struct result;

typedef struct result (*builtin_function)(struct object *, struct environment *, struct environment *);

void insert_builtins(struct environment *env);
