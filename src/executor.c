#include "executor.h"

#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message) {
    if (error == NULL || error_size == 0) {
        return;
    }

    snprintf(error, error_size, "%s", message);
}

static char *duplicate_string(const char *value) {
    size_t length = strlen(value);
    char *copy = (char *) malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, value, length + 1);
    return copy;
}

int execute_command(const SqlCommand *command, char **output, char *error, size_t error_size) {
    if (command == NULL || output == NULL) {
        set_error(error, error_size, "Command execution input is invalid.");
        return 0;
    }

    *output = NULL;

    if (command->type == SQL_COMMAND_INSERT) {
        if (!storage_append_row(command, error, error_size)) {
            return 0;
        }

        *output = duplicate_string("INSERT OK\n");
        if (*output == NULL) {
            set_error(error, error_size, "Out of memory while building INSERT output.");
            return 0;
        }

        return 1;
    }

    if (command->type == SQL_COMMAND_SELECT) {
        return storage_select_rows(command, output, error, error_size);
    }

    set_error(error, error_size, "Unsupported command type.");
    return 0;
}
