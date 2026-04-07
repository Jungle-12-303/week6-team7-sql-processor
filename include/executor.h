#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stddef.h>

#include "command.h"

int execute_command(const SqlCommand *command, char **output, char *error, size_t error_size);

#endif
