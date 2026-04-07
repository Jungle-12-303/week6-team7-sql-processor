#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "command.h"

int parse_sql(const char *sql, SqlCommand *command, char *error, size_t error_size);

#endif
