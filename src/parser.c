#include "parser.h"

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

static char *duplicate_range(const char *start, const char *end) {
    size_t length = 0;
    char *copy = NULL;

    if (start == NULL || end == NULL || end < start) {
        return NULL;
    }

    length = (size_t) (end - start);
    copy = (char *) malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, start, length);
    copy[length] = '\0';
    return copy;
}

static void trim_span(const char **start, const char **end) {
    while (*start < *end && isspace((unsigned char) **start)) {
        (*start)++;
    }

    while (*end > *start && isspace((unsigned char) *(*end - 1))) {
        (*end)--;
    }
}

static void skip_spaces(const char **cursor) {
    while (**cursor != '\0' && isspace((unsigned char) **cursor)) {
        (*cursor)++;
    }
}

static int chars_equal_ignore_case(char left, char right) {
    return tolower((unsigned char) left) == tolower((unsigned char) right);
}

static int starts_with_keyword(const char *text, const char *keyword) {
    size_t index = 0;

    for (index = 0; keyword[index] != '\0'; ++index) {
        if (text[index] == '\0' || !chars_equal_ignore_case(text[index], keyword[index])) {
            return 0;
        }
    }

    return !(isalnum((unsigned char) text[index]) || text[index] == '_');
}

static int consume_keyword(const char **cursor, const char *keyword) {
    const char *probe = *cursor;

    skip_spaces(&probe);
    if (!starts_with_keyword(probe, keyword)) {
        return 0;
    }

    *cursor = probe + strlen(keyword);
    return 1;
}

static int is_identifier_start(char value) {
    return isalpha((unsigned char) value) || value == '_';
}

static int is_identifier_part(char value) {
    return isalnum((unsigned char) value) || value == '_';
}

static int parse_identifier(const char **cursor, char **identifier, char *error, size_t error_size) {
    const char *start = NULL;
    const char *end = NULL;

    skip_spaces(cursor);
    start = *cursor;
    end = start;

    if (!is_identifier_start(*start)) {
        set_error(error, error_size, "Expected identifier.");
        return 0;
    }

    while (is_identifier_part(*end)) {
        end++;
    }

    *identifier = duplicate_range(start, end);
    if (*identifier == NULL) {
        set_error(error, error_size, "Out of memory while parsing identifier.");
        return 0;
    }

    *cursor = end;
    return 1;
}

static int append_string(char ***items, size_t *count, char *value, char *error, size_t error_size) {
    char **grown = NULL;

    grown = (char **) realloc(*items, sizeof(char *) * (*count + 1));
    if (grown == NULL) {
        free(value);
        set_error(error, error_size, "Out of memory while growing list.");
        return 0;
    }

    grown[*count] = value;
    *items = grown;
    (*count)++;
    return 1;
}

static int parse_identifier_list_in_parentheses(const char **cursor, char ***items, size_t *count, char *error, size_t error_size) {
    char *identifier = NULL;

    skip_spaces(cursor);
    if (**cursor != '(') {
        set_error(error, error_size, "Expected '('.");
        return 0;
    }

    (*cursor)++;

    for (;;) {
        if (!parse_identifier(cursor, &identifier, error, error_size)) {
            return 0;
        }

        if (!append_string(items, count, identifier, error, error_size)) {
            return 0;
        }

        skip_spaces(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }

        if (**cursor == ')') {
            (*cursor)++;
            return 1;
        }

        set_error(error, error_size, "Expected ',' or ')' in identifier list.");
        return 0;
    }
}

static int parse_select_columns(const char **cursor, char ***items, size_t *count, char *error, size_t error_size) {
    char *identifier = NULL;
    const char *probe = NULL;

    for (;;) {
        if (!parse_identifier(cursor, &identifier, error, error_size)) {
            return 0;
        }

        if (!append_string(items, count, identifier, error, error_size)) {
            return 0;
        }

        skip_spaces(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }

        probe = *cursor;
        if (consume_keyword(&probe, "FROM")) {
            *cursor = probe;
            return 1;
        }

        set_error(error, error_size, "Expected ',' or FROM in SELECT statement.");
        return 0;
    }
}

static int parse_value(const char **cursor, char **value, char *error, size_t error_size) {
    const char *start = NULL;
    const char *end = NULL;

    skip_spaces(cursor);

    if (**cursor == '\'') {
        start = *cursor + 1;
        end = start;

        while (*end != '\0' && *end != '\'') {
            if (*end == '\n' || *end == '\r') {
                set_error(error, error_size, "String values cannot contain newlines.");
                return 0;
            }
            end++;
        }

        if (*end != '\'') {
            set_error(error, error_size, "Unterminated string literal.");
            return 0;
        }

        *value = duplicate_range(start, end);
        if (*value == NULL) {
            set_error(error, error_size, "Out of memory while parsing string value.");
            return 0;
        }

        *cursor = end + 1;
        return 1;
    }

    start = *cursor;
    end = start;
    while (*end != '\0' && *end != ',' && *end != ')') {
        end++;
    }

    trim_span(&start, &end);
    if (start == end) {
        set_error(error, error_size, "Expected value.");
        return 0;
    }

    *value = duplicate_range(start, end);
    if (*value == NULL) {
        set_error(error, error_size, "Out of memory while parsing value.");
        return 0;
    }

    *cursor = end;
    return 1;
}

static int parse_values_in_parentheses(const char **cursor, char ***items, size_t *count, char *error, size_t error_size) {
    char *value = NULL;

    skip_spaces(cursor);
    if (**cursor != '(') {
        set_error(error, error_size, "Expected '(' before VALUES list.");
        return 0;
    }

    (*cursor)++;

    for (;;) {
        if (!parse_value(cursor, &value, error, error_size)) {
            return 0;
        }

        if (!append_string(items, count, value, error, error_size)) {
            return 0;
        }

        skip_spaces(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }

        if (**cursor == ')') {
            (*cursor)++;
            return 1;
        }

        set_error(error, error_size, "Expected ',' or ')' in VALUES list.");
        return 0;
    }
}

static int ensure_end_of_input(const char **cursor, char *error, size_t error_size) {
    skip_spaces(cursor);
    if (**cursor != '\0') {
        set_error(error, error_size, "Unexpected trailing input.");
        return 0;
    }

    return 1;
}

static int extract_statement(const char *sql, char **statement, char *error, size_t error_size) {
    const char *first_non_space = NULL;
    const char *semicolon = NULL;
    const char *cursor = NULL;
    int in_string = 0;

    if (sql == NULL) {
        set_error(error, error_size, "SQL input is null.");
        return 0;
    }

    cursor = sql;
    while (*cursor != '\0') {
        if (!isspace((unsigned char) *cursor) && first_non_space == NULL) {
            first_non_space = cursor;
        }

        if (*cursor == '\'') {
            in_string = !in_string;
        } else if (*cursor == ';' && !in_string) {
            if (semicolon != NULL) {
                set_error(error, error_size, "Only one SQL statement is supported.");
                return 0;
            }
            semicolon = cursor;
        }

        cursor++;
    }

    if (first_non_space == NULL) {
        set_error(error, error_size, "SQL input is empty.");
        return 0;
    }

    if (semicolon == NULL) {
        set_error(error, error_size, "Statement must end with ';'.");
        return 0;
    }

    cursor = semicolon + 1;
    while (*cursor != '\0') {
        if (!isspace((unsigned char) *cursor)) {
            set_error(error, error_size, "Only one SQL statement is supported.");
            return 0;
        }
        cursor++;
    }

    *statement = duplicate_range(sql, semicolon);
    if (*statement == NULL) {
        set_error(error, error_size, "Out of memory while copying statement.");
        return 0;
    }

    first_non_space = *statement;
    cursor = *statement + strlen(*statement);
    trim_span(&first_non_space, &cursor);
    if (first_non_space == cursor) {
        free(*statement);
        *statement = NULL;
        set_error(error, error_size, "SQL input is empty.");
        return 0;
    }

    {
        char *trimmed = duplicate_range(first_non_space, cursor);
        free(*statement);
        *statement = trimmed;
    }

    if (*statement == NULL) {
        set_error(error, error_size, "Out of memory while trimming statement.");
        return 0;
    }

    return 1;
}

static int parse_insert(const char *statement, SqlCommand *command, char *error, size_t error_size) {
    const char *cursor = statement;

    command->type = SQL_COMMAND_INSERT;

    if (!consume_keyword(&cursor, "INSERT")) {
        set_error(error, error_size, "Expected INSERT keyword.");
        return 0;
    }

    if (!consume_keyword(&cursor, "INTO")) {
        set_error(error, error_size, "Expected INTO keyword.");
        return 0;
    }

    if (!parse_identifier(&cursor, &command->table_name, error, error_size)) {
        return 0;
    }

    if (!parse_identifier_list_in_parentheses(&cursor, &command->columns, &command->column_count, error, error_size)) {
        return 0;
    }

    if (!consume_keyword(&cursor, "VALUES")) {
        set_error(error, error_size, "Expected VALUES keyword.");
        return 0;
    }

    if (!parse_values_in_parentheses(&cursor, &command->values, &command->value_count, error, error_size)) {
        return 0;
    }

    if (command->column_count != command->value_count) {
        set_error(error, error_size, "Column count and value count must match.");
        return 0;
    }

    return ensure_end_of_input(&cursor, error, error_size);
}

static int parse_select(const char *statement, SqlCommand *command, char *error, size_t error_size) {
    const char *cursor = statement;

    command->type = SQL_COMMAND_SELECT;

    if (!consume_keyword(&cursor, "SELECT")) {
        set_error(error, error_size, "Expected SELECT keyword.");
        return 0;
    }

    if (!parse_select_columns(&cursor, &command->columns, &command->column_count, error, error_size)) {
        return 0;
    }

    if (!parse_identifier(&cursor, &command->table_name, error, error_size)) {
        return 0;
    }

    return ensure_end_of_input(&cursor, error, error_size);
}

int parse_sql(const char *sql, SqlCommand *command, char *error, size_t error_size) {
    char *statement = NULL;
    int ok = 0;

    if (command == NULL) {
        set_error(error, error_size, "Command output cannot be null.");
        return 0;
    }

    sql_command_init(command);

    if (!extract_statement(sql, &statement, error, error_size)) {
        return 0;
    }

    if (starts_with_keyword(statement, "INSERT")) {
        ok = parse_insert(statement, command, error, error_size);
    } else if (starts_with_keyword(statement, "SELECT")) {
        ok = parse_select(statement, command, error, error_size);
    } else {
        set_error(error, error_size, "Unsupported SQL statement.");
        ok = 0;
    }

    free(statement);

    if (!ok) {
        sql_command_free(command);
    }

    return ok;
}
