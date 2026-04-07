#include "storage.h"

#include <ctype.h>
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
    size_t length = 0;
    char *copy = NULL;

    if (value == NULL) {
        return NULL;
    }

    length = strlen(value);
    copy = (char *) malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, value, length + 1);
    return copy;
}

static int equals_ignore_case(const char *left, const char *right) {
    size_t index = 0;

    if (left == NULL || right == NULL) {
        return 0;
    }

    while (left[index] != '\0' && right[index] != '\0') {
        if (tolower((unsigned char) left[index]) != tolower((unsigned char) right[index])) {
            return 0;
        }
        index++;
    }

    return left[index] == '\0' && right[index] == '\0';
}

static void trim_newline(char *value) {
    size_t length = 0;

    if (value == NULL) {
        return;
    }

    length = strlen(value);
    while (length > 0 && (value[length - 1] == '\n' || value[length - 1] == '\r')) {
        value[length - 1] = '\0';
        length--;
    }
}

static char *build_data_path(const char *table_name) {
    size_t length = 0;
    char *path = NULL;

    length = strlen("data/") + strlen(table_name) + strlen(".csv") + 1;
    path = (char *) malloc(length);
    if (path == NULL) {
        return NULL;
    }

    snprintf(path, length, "data/%s.csv", table_name);
    return path;
}

static char *read_line_alloc(FILE *file) {
    size_t capacity = 128;
    size_t length = 0;
    int ch = 0;
    char *buffer = (char *) malloc(capacity);

    if (buffer == NULL) {
        return NULL;
    }

    while ((ch = fgetc(file)) != EOF) {
        if (length + 1 >= capacity) {
            char *grown = NULL;
            capacity *= 2;
            grown = (char *) realloc(buffer, capacity);
            if (grown == NULL) {
                free(buffer);
                return NULL;
            }
            buffer = grown;
        }

        buffer[length++] = (char) ch;
        if (ch == '\n') {
            break;
        }
    }

    if (length == 0 && ch == EOF) {
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';
    return buffer;
}

static int split_csv_line(const char *line, char ***items, size_t *count, char *error, size_t error_size) {
    const char *start = NULL;
    const char *cursor = NULL;
    char *clean = NULL;
    char *field = NULL;
    char **grown = NULL;

    *items = NULL;
    *count = 0;

    clean = duplicate_string(line);
    if (clean == NULL) {
        set_error(error, error_size, "Out of memory while splitting CSV.");
        return 0;
    }

    trim_newline(clean);
    start = clean;
    cursor = clean;

    while (1) {
        if (*cursor == ',' || *cursor == '\0') {
            field = (char *) malloc((size_t) (cursor - start) + 1);
            if (field == NULL) {
                free(clean);
                set_error(error, error_size, "Out of memory while copying CSV field.");
                return 0;
            }

            memcpy(field, start, (size_t) (cursor - start));
            field[cursor - start] = '\0';

            grown = (char **) realloc(*items, sizeof(char *) * (*count + 1));
            if (grown == NULL) {
                free(field);
                free(clean);
                set_error(error, error_size, "Out of memory while growing CSV fields.");
                return 0;
            }

            *items = grown;
            (*items)[*count] = field;
            (*count)++;

            if (*cursor == '\0') {
                break;
            }

            cursor++;
            start = cursor;
            continue;
        }

        cursor++;
    }

    free(clean);
    return 1;
}

static void free_string_list(char **items, size_t count) {
    size_t index = 0;

    if (items == NULL) {
        return;
    }

    for (index = 0; index < count; ++index) {
        free(items[index]);
    }

    free(items);
}

static int find_column_index(char **columns, size_t column_count, const char *target) {
    size_t index = 0;

    for (index = 0; index < column_count; ++index) {
        if (equals_ignore_case(columns[index], target)) {
            return (int) index;
        }
    }

    return -1;
}

static int validate_insert_value(const char *value, char *error, size_t error_size) {
    if (value == NULL) {
        set_error(error, error_size, "INSERT value cannot be null.");
        return 0;
    }

    if (strchr(value, ',') != NULL || strchr(value, '\n') != NULL || strchr(value, '\r') != NULL) {
        set_error(error, error_size, "Values cannot contain commas or newlines in minimal CSV mode.");
        return 0;
    }

    return 1;
}

static int file_needs_newline(FILE *file) {
    long size = 0;
    int last_char = 0;

    if (fseek(file, 0, SEEK_END) != 0) {
        return 1;
    }

    size = ftell(file);
    if (size <= 0) {
        return 0;
    }

    if (fseek(file, -1L, SEEK_END) != 0) {
        return 1;
    }

    last_char = fgetc(file);
    return last_char != '\n';
}

static int append_text(char **buffer, size_t *length, size_t *capacity, const char *text, char *error, size_t error_size) {
    size_t text_length = strlen(text);
    size_t required = *length + text_length + 1;
    char *grown = NULL;

    if (*buffer == NULL) {
        *capacity = required + 64;
        *buffer = (char *) malloc(*capacity);
        if (*buffer == NULL) {
            set_error(error, error_size, "Out of memory while building output.");
            return 0;
        }
        (*buffer)[0] = '\0';
    } else if (required > *capacity) {
        *capacity = required + 64;
        grown = (char *) realloc(*buffer, *capacity);
        if (grown == NULL) {
            set_error(error, error_size, "Out of memory while growing output.");
            return 0;
        }
        *buffer = grown;
    }

    memcpy(*buffer + *length, text, text_length + 1);
    *length += text_length;
    return 1;
}

int storage_append_row(const SqlCommand *command, char *error, size_t error_size) {
    char *path = NULL;
    FILE *read_file = NULL;
    FILE *append_file = NULL;
    char *header_line = NULL;
    char **header_columns = NULL;
    char **ordered_values = NULL;
    size_t header_count = 0;
    size_t index = 0;
    int needs_newline = 0;
    int ok = 0;

    if (command == NULL || command->table_name == NULL) {
        set_error(error, error_size, "Command or table name is missing.");
        return 0;
    }

    path = build_data_path(command->table_name);
    if (path == NULL) {
        set_error(error, error_size, "Out of memory while building table path.");
        return 0;
    }

    read_file = fopen(path, "rb");
    if (read_file == NULL) {
        set_error(error, error_size, "Target table file does not exist.");
        goto cleanup;
    }

    header_line = read_line_alloc(read_file);
    if (header_line == NULL) {
        set_error(error, error_size, "Table file is empty.");
        goto cleanup;
    }

    if (!split_csv_line(header_line, &header_columns, &header_count, error, error_size)) {
        goto cleanup;
    }

    if (header_count != command->column_count) {
        set_error(error, error_size, "INSERT must provide exactly all table columns.");
        goto cleanup;
    }

    ordered_values = (char **) calloc(header_count, sizeof(char *));
    if (ordered_values == NULL) {
        set_error(error, error_size, "Out of memory while ordering row values.");
        goto cleanup;
    }

    for (index = 0; index < header_count; ++index) {
        int command_index = find_column_index(command->columns, command->column_count, header_columns[index]);
        if (command_index < 0) {
            set_error(error, error_size, "INSERT column list must match table header.");
            goto cleanup;
        }

        if (!validate_insert_value(command->values[command_index], error, error_size)) {
            goto cleanup;
        }

        ordered_values[index] = command->values[command_index];
    }

    needs_newline = file_needs_newline(read_file);
    fclose(read_file);
    read_file = NULL;

    append_file = fopen(path, "ab");
    if (append_file == NULL) {
        set_error(error, error_size, "Failed to open table file for appending.");
        goto cleanup;
    }

    if (needs_newline) {
        fputs("\n", append_file);
    }

    for (index = 0; index < header_count; ++index) {
        if (index > 0) {
            fputs(",", append_file);
        }
        fputs(ordered_values[index], append_file);
    }
    fputs("\n", append_file);

    ok = 1;

cleanup:
    free(path);
    free(header_line);
    free_string_list(header_columns, header_count);
    free(ordered_values);
    if (read_file != NULL) {
        fclose(read_file);
    }
    if (append_file != NULL) {
        fclose(append_file);
    }
    return ok;
}

int storage_select_rows(const SqlCommand *command, char **output, char *error, size_t error_size) {
    char *path = NULL;
    FILE *file = NULL;
    char *header_line = NULL;
    char *row_line = NULL;
    char **header_columns = NULL;
    char **row_columns = NULL;
    int *requested_indices = NULL;
    size_t header_count = 0;
    size_t row_count = 0;
    size_t requested_index = 0;
    size_t output_length = 0;
    size_t output_capacity = 0;
    int ok = 0;

    if (output == NULL) {
        set_error(error, error_size, "Output pointer is missing.");
        return 0;
    }

    *output = NULL;

    path = build_data_path(command->table_name);
    if (path == NULL) {
        set_error(error, error_size, "Out of memory while building table path.");
        return 0;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        set_error(error, error_size, "Target table file does not exist.");
        goto cleanup;
    }

    header_line = read_line_alloc(file);
    if (header_line == NULL) {
        set_error(error, error_size, "Table file is empty.");
        goto cleanup;
    }

    if (!split_csv_line(header_line, &header_columns, &header_count, error, error_size)) {
        goto cleanup;
    }

    requested_indices = (int *) malloc(sizeof(int) * command->column_count);
    if (requested_indices == NULL) {
        set_error(error, error_size, "Out of memory while mapping SELECT columns.");
        goto cleanup;
    }

    for (requested_index = 0; requested_index < command->column_count; ++requested_index) {
        requested_indices[requested_index] = find_column_index(header_columns, header_count, command->columns[requested_index]);
        if (requested_indices[requested_index] < 0) {
            set_error(error, error_size, "Requested column does not exist in table.");
            goto cleanup;
        }
    }

    for (requested_index = 0; requested_index < command->column_count; ++requested_index) {
        if (requested_index > 0 && !append_text(output, &output_length, &output_capacity, ",", error, error_size)) {
            goto cleanup;
        }

        if (!append_text(output, &output_length, &output_capacity, command->columns[requested_index], error, error_size)) {
            goto cleanup;
        }
    }

    if (!append_text(output, &output_length, &output_capacity, "\n", error, error_size)) {
        goto cleanup;
    }

    while ((row_line = read_line_alloc(file)) != NULL) {
        if (!split_csv_line(row_line, &row_columns, &row_count, error, error_size)) {
            goto cleanup;
        }

        if (row_count == 1 && row_columns[0][0] == '\0') {
            free(row_line);
            row_line = NULL;
            free_string_list(row_columns, row_count);
            row_columns = NULL;
            row_count = 0;
            continue;
        }

        if (row_count != header_count) {
            set_error(error, error_size, "CSV row does not match header column count.");
            goto cleanup;
        }

        for (requested_index = 0; requested_index < command->column_count; ++requested_index) {
            if (requested_index > 0 && !append_text(output, &output_length, &output_capacity, ",", error, error_size)) {
                goto cleanup;
            }

            if (!append_text(output, &output_length, &output_capacity, row_columns[requested_indices[requested_index]], error, error_size)) {
                goto cleanup;
            }
        }

        if (!append_text(output, &output_length, &output_capacity, "\n", error, error_size)) {
            goto cleanup;
        }

        free(row_line);
        row_line = NULL;
        free_string_list(row_columns, row_count);
        row_columns = NULL;
        row_count = 0;
    }

    ok = 1;

cleanup:
    free(path);
    free(header_line);
    free_string_list(header_columns, header_count);
    free(requested_indices);
    free(row_line);
    free_string_list(row_columns, row_count);

    if (!ok) {
        free(*output);
        *output = NULL;
    }

    if (file != NULL) {
        fclose(file);
    }

    return ok;
}
