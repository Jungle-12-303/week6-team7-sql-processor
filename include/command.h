#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

typedef enum SqlCommandType {
    SQL_COMMAND_INSERT = 0,
    SQL_COMMAND_SELECT = 1
} SqlCommandType;

typedef struct SqlCommand {
    SqlCommandType type;
    char *table_name;
    char **columns;
    size_t column_count;
    char **values;
    size_t value_count;
} SqlCommand;

void sql_command_init(SqlCommand *command);
void sql_command_free(SqlCommand *command);

#endif
