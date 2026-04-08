#include "cli_runner.h"

#include "command.h"
#include "executor.h"
#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_LINE_BUFFER_SIZE 4096

/*
 * SQL 입력 파일 전체를 메모리 문자열로 읽어 오는 함수다.
 *
 * 파일 입력 모드에서는 SQL 한 문장을 먼저 문자열로 읽은 뒤 Parser로 넘기므로,
 * 이 함수가 CLI와 Parser 사이의 입력 준비 단계를 담당한다.
 */
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

/*
 * 이미 문자열로 준비된 SQL 한 문장을 끝까지 실행한다.
 *
 * 이 함수는 "파싱 -> 실행 -> 출력"의 공통 흐름을 묶어 두기 위해 존재한다.
 * 파일 입력 모드와 interactive 모드가 모두 이 함수를 재사용하므로,
 * 실제 SQL 처리의 중심 연결점이라고 볼 수 있다.
 */
static int execute_sql_text(const char *sql, FILE *out_stream, FILE *err_stream) {
    char error[256];
    char *output = NULL;
    SqlCommand command;
    int success = 0;

    sql_command_init(&command);
    error[0] = '\0';

    if (!parse_sql(sql, &command, error, sizeof(error))) {
        fprintf(err_stream, "ERROR: %s\n", error);
        sql_command_free(&command);
        return 1;
    }

    success = execute_command(&command, &output, error, sizeof(error));
    if (!success) {
        fprintf(err_stream, "ERROR: %s\n", error);
        sql_command_free(&command);
        free(output);
        return 1;
    }

    fputs(output, out_stream);

    sql_command_free(&command);
    free(output);
    return 0;
}

/*
 * interactive 모드 입력이 종료 명령인지 검사한다.
 *
 * 앞뒤 공백을 무시하고 exit 또는 quit만 종료 명령으로 인정한다.
 * 사용자가 "exit   "처럼 공백을 붙여 입력해도 자연스럽게 종료되도록 처리한다.
 */
static int is_exit_command(const char *line) {
    const char *start = line;
    const char *end = NULL;
    size_t length = 0;

    while (*start != '\0' && isspace((unsigned char) *start)) {
        start++;
    }

    end = start + strlen(start);
    while (end > start && isspace((unsigned char) *(end - 1))) {
        end--;
    }

    length = (size_t) (end - start);
    return (length == 4 && strncmp(start, "exit", length) == 0)
        || (length == 4 && strncmp(start, "quit", length) == 0);
}

/*
 * interactive CLI 모드를 실행한다.
 *
 * 한 줄씩 SQL을 읽고, 빈 줄은 건너뛰며, 종료 명령이 들어오면 루프를 끝낸다.
 * SQL 문자열이 들어오면 execute_sql_text를 호출해 같은 실행 파이프라인으로 넘긴다.
 */
int run_cli_interactive_with_streams(FILE *input_stream, FILE *out_stream, FILE *err_stream) {
    char line[CLI_LINE_BUFFER_SIZE];

    fprintf(out_stream, "Interactive SQL mode. Type 'exit' to quit.\n");

    while (1) {
        size_t length = 0;

        fprintf(out_stream, "sql> ");
        if (fgets(line, sizeof(line), input_stream) == NULL) {
            fprintf(out_stream, "\n");
            return 0;
        }

        length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[length - 1] = '\0';
            length--;
        }

        if (line[0] == '\0') {
            continue;
        }

        if (is_exit_command(line)) {
            return 0;
        }

        execute_sql_text(line, out_stream, err_stream);
    }
}

/*
 * CLI의 실제 진입 함수다.
 *
 * 인자가 없으면 interactive 모드로 들어가고,
 * 인자가 하나면 해당 SQL 파일을 읽어 실행한다.
 * 잘못된 인자 개수나 파일 열기 실패는 이 계층에서 바로 사용자에게 알려 준다.
 */
int run_cli_with_streams(int argc, char **argv, FILE *out_stream, FILE *err_stream) {
    char error[256];
    char *sql = NULL;
    int result = 0;

    if (argc == 1) {
        return run_cli_interactive_with_streams(stdin, out_stream, err_stream);
    }

    if (argc != 2) {
        fprintf(err_stream, "Usage: sql_processor <sql-file>\n");
        return 1;
    }

    if (!read_file_to_string(argv[1], &sql, error, sizeof(error))) {
        fprintf(err_stream, "ERROR: %s\n", error);
        return 1;
    }

    result = execute_sql_text(sql, out_stream, err_stream);
    free(sql);
    return result;
}

/*
 * 실제 표준 입출력을 사용하는 기본 CLI 진입 함수다.
 *
 * 테스트에서는 run_cli_with_streams를 사용해 출력 스트림을 바꾸고,
 * 일반 실행에서는 이 함수가 stdout/stderr를 그대로 넘겨 준다.
 */
int run_cli(int argc, char **argv) {
    return run_cli_with_streams(argc, argv, stdout, stderr);
}
