#pragma once

#include <stdbool.h>

bool ced_init(long long stack_size);
bool ced_eval(const char *code);
const char *ced_error(void);
