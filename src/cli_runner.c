#include "cli_runner.h"

#include "command.h"
#include "executor.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

static int read_file_to_string(const char *path, char **contents, char *error, size_t error_size) {
    FILE *file = NULL;
    long length = 0;
    char *buffer = NULL;

    *contents = NULL;

    file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(error, error_size, "%s", "Failed to open SQL input file.");
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        snprintf(error, error_size, "%s", "Failed to read SQL input file.");
        return 0;
    }

    length = ftell(file);
    if (length < 0) {
        fclose(file);
        snprintf(error, error_size, "%s", "Failed to read SQL input file.");
        return 0;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        snprintf(error, error_size, "%s", "Failed to read SQL input file.");
        return 0;
    }

    buffer = (char *) malloc((size_t) length + 1);
    if (buffer == NULL) {
        fclose(file);
        snprintf(error, error_size, "%s", "Out of memory while reading SQL input file.");
        return 0;
    }

    if (length > 0 && fread(buffer, 1, (size_t) length, file) != (size_t) length) {
        free(buffer);
        fclose(file);
        snprintf(error, error_size, "%s", "Failed to read SQL input file.");
        return 0;
    }

    buffer[length] = '\0';
    fclose(file);
    *contents = buffer;
    return 1;
}

int run_cli_with_streams(int argc, char **argv, FILE *out_stream, FILE *err_stream) {
    char error[256];
    char *sql = NULL;
    char *output = NULL;
    SqlCommand command;
    int success = 0;

    sql_command_init(&command);
    error[0] = '\0';

    if (argc != 2) {
        fprintf(err_stream, "Usage: sql_processor <sql-file>\n");
        return 1;
    }

    if (!read_file_to_string(argv[1], &sql, error, sizeof(error))) {
        fprintf(err_stream, "ERROR: %s\n", error);
        return 1;
    }

    if (!parse_sql(sql, &command, error, sizeof(error))) {
        fprintf(err_stream, "ERROR: %s\n", error);
        free(sql);
        return 1;
    }

    success = execute_command(&command, &output, error, sizeof(error));
    if (!success) {
        fprintf(err_stream, "ERROR: %s\n", error);
        sql_command_free(&command);
        free(sql);
        free(output);
        return 1;
    }

    fputs(output, out_stream);

    sql_command_free(&command);
    free(sql);
    free(output);
    return 0;
}

int run_cli(int argc, char **argv) {
    return run_cli_with_streams(argc, argv, stdout, stderr);
}
