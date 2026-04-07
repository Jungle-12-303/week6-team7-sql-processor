#include "command.h"

#include <stdlib.h>

void sql_command_init(SqlCommand *command) {
    if (command == NULL) {
        return;
    }

    command->type = SQL_COMMAND_INSERT;
    command->table_name = NULL;
    command->columns = NULL;
    command->column_count = 0;
    command->values = NULL;
    command->value_count = 0;
}

void sql_command_free(SqlCommand *command) {
    size_t index = 0;

    if (command == NULL) {
        return;
    }

    free(command->table_name);

    if (command->columns != NULL) {
        for (index = 0; index < command->column_count; ++index) {
            free(command->columns[index]);
        }
    }

    if (command->values != NULL) {
        for (index = 0; index < command->value_count; ++index) {
            free(command->values[index]);
        }
    }

    free(command->columns);
    free(command->values);

    sql_command_init(command);
}
