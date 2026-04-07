#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

#include "command.h"

int storage_append_row(const SqlCommand *command, char *error, size_t error_size);
int storage_select_rows(const SqlCommand *command, char **output, char *error, size_t error_size);

#endif
