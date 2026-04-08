#include "storage.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Storage 계층 내부에서 사용할 오류 메시지를 안전하게 복사한다.
 *
 * 오류 버퍼가 없거나 길이가 0이면 아무 작업도 하지 않고,
 * 그렇지 않으면 전달받은 메시지를 잘라내지 않는 범위에서 복사한다.
 * Storage 내부 보조 함수들이 공통적으로 호출하는 가장 기본적인 오류 기록 함수다.
 */
static void set_error(char *error, size_t error_size, const char *message) {
    if (error == NULL || error_size == 0) {
        return;
    }

    snprintf(error, error_size, "%s", message);
}

/*
 * 널 종료 문자열을 새 메모리에 복제한다.
 *
 * SELECT 결과 행을 별도 메모리에 저장하거나,
 * CSV 한 줄에서 잘라낸 필드를 독립적으로 보관할 때 사용한다.
 * 입력이 NULL이면 NULL을 반환하고, 메모리 할당 실패 시에도 NULL을 반환한다.
 */
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

/*
 * 두 문자열이 대소문자를 무시하면 같은지 확인한다.
 *
 * CSV 헤더와 SQL 컬럼 이름을 비교할 때 사용하며,
 * 사용자가 `ID`, `Id`, `id`처럼 다른 대소문자로 입력해도 같은 컬럼으로 취급하게 해 준다.
 */
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

/*
 * 파일에서 읽어 온 줄 끝의 개행 문자를 제거한다.
 *
 * `fgets`나 직접 읽기 함수로 가져온 문자열에는 `\n`, `\r\n`이 포함될 수 있다.
 * CSV 헤더와 데이터 값을 안정적으로 비교하려면 먼저 줄 끝 개행을 제거해야 한다.
 */
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

/*
 * 테이블 이름으로 실제 CSV 파일 경로를 만든다.
 *
 * 현재 프로젝트는 각 테이블을 `data/<table>.csv` 파일 하나로 저장한다.
 * 이 함수는 SQL에서 넘어온 테이블 이름을 파일 시스템 경로로 변환하는 Storage 계층의 진입점이다.
 */
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

/*
 * 파일에서 한 줄을 읽어 동적 메모리 문자열로 반환한다.
 *
 * 줄 길이를 미리 알 수 없으므로 버퍼를 점진적으로 늘리면서 읽는다.
 * 개행을 만나면 그 줄에서 읽기를 멈추고, EOF에서 더 이상 읽을 내용이 없으면 NULL을 반환한다.
 * header 한 줄, 각 데이터 행 한 줄을 처리하는 공통 유틸리티다.
 */
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

/*
 * 최소 CSV 규칙에 따라 한 줄을 쉼표 기준으로 분리한다.
 *
 * 이 프로젝트는 quoted CSV를 지원하지 않으므로 단순히 `,`를 기준으로 나눈다.
 * 각 필드는 새 메모리로 복사되어 반환되며, 호출자는 free_string_list로 해제해야 한다.
 * 메모리 할당이 실패하면 오류 메시지를 기록하고 0을 반환한다.
 */
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

/*
 * 문자열 배열과 그 안의 각 항목을 모두 해제한다.
 *
 * CSV 헤더 목록, 한 행의 필드 목록, SELECT 결과로 모아 둔 행 목록 등
 * Storage 계층 곳곳에서 같은 메모리 해제 패턴이 반복되므로 별도 함수로 분리했다.
 */
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

/*
 * 컬럼 이름 목록에서 원하는 컬럼의 위치를 찾는다.
 *
 * 헤더 컬럼 순서와 SQL에서 요청한 컬럼 순서가 다를 수 있으므로,
 * 실제 CSV 인덱스를 찾은 뒤 그 순서에 맞게 INSERT 값을 재배치하거나 SELECT 값을 추출한다.
 */
static int find_column_index(char **columns, size_t column_count, const char *target) {
    size_t index = 0;

    for (index = 0; index < column_count; ++index) {
        if (equals_ignore_case(columns[index], target)) {
            return (int) index;
        }
    }

    return -1;
}

/*
 * INSERT 값이 현재의 최소 CSV 저장 규칙에 맞는지 검사한다.
 *
 * 이 프로젝트는 CSV escaping을 구현하지 않았기 때문에
 * 값 안에 쉼표나 개행이 들어가면 파일 구조가 깨질 수 있다.
 * 그래서 저장 전에 이러한 값을 미리 차단한다.
 */
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

/*
 * 파일 끝에 새 행을 붙이기 전에 개행이 필요한지 확인한다.
 *
 * 기존 파일이 개행 없이 끝났다면 바로 데이터를 이어 쓰면
 * 마지막 기존 행과 새 행이 붙어 버리므로, append 전에 `\n`을 하나 넣어야 한다.
 */
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

/*
 * 가변 길이 출력 버퍼 뒤에 문자열을 이어 붙인다.
 *
 * SELECT 결과는 행 수와 값 길이를 미리 알 수 없으므로
 * 동적으로 커지는 문자열 버퍼에 표 형태 텍스트를 순서대로 쌓아 만든다.
 */
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

/*
 * 같은 문자를 여러 번 출력 버퍼에 추가한다.
 *
 * 표의 가로 경계선(`----`)이나 값 오른쪽 padding 공백처럼
 * 동일한 문자를 반복해 붙여야 할 때 사용한다.
 */
static int append_repeated_char(
    char **buffer,
    size_t *length,
    size_t *capacity,
    char ch,
    size_t count,
    char *error,
    size_t error_size
) {
    size_t index = 0;

    for (index = 0; index < count; ++index) {
        char text[2];
        text[0] = ch;
        text[1] = '\0';
        if (!append_text(buffer, length, capacity, text, error, error_size)) {
            return 0;
        }
    }

    return 1;
}

/*
 * 값을 지정된 폭만큼 왼쪽 정렬로 출력하고 부족한 폭은 공백으로 채운다.
 *
 * 각 컬럼의 최대 너비에 맞춰 모든 행이 같은 폭으로 보이게 만드는 함수다.
 * 예를 들어 `id` 컬럼 폭이 4면 `1` 뒤에 공백 3칸을 붙여 표 모양을 맞춘다.
 */
static int append_padded_value(
    char **buffer,
    size_t *length,
    size_t *capacity,
    const char *value,
    size_t width,
    char *error,
    size_t error_size
) {
    size_t value_length = strlen(value);

    if (!append_text(buffer, length, capacity, value, error, error_size)) {
        return 0;
    }

    if (width > value_length && !append_repeated_char(buffer, length, capacity, ' ', width - value_length, error, error_size)) {
        return 0;
    }

    return 1;
}

/*
 * 표의 위/아래 경계선 한 줄을 만든다.
 *
 * 컬럼 폭이 `[2, 5]`라면 `+----+-------+` 형태의 줄을 생성한다.
 * 헤더 위, 헤더 아래, 마지막 행 아래에서 동일한 함수를 사용한다.
 */
static int append_table_border(
    char **buffer,
    size_t *length,
    size_t *capacity,
    const size_t *widths,
    size_t column_count,
    char *error,
    size_t error_size
) {
    size_t index = 0;

    if (!append_text(buffer, length, capacity, "+", error, error_size)) {
        return 0;
    }

    for (index = 0; index < column_count; ++index) {
        if (!append_repeated_char(buffer, length, capacity, '-', widths[index] + 2, error, error_size)) {
            return 0;
        }

        if (!append_text(buffer, length, capacity, "+", error, error_size)) {
            return 0;
        }
    }

    return append_text(buffer, length, capacity, "\n", error, error_size);
}

/*
 * 표의 데이터 행 또는 헤더 행 한 줄을 만든다.
 *
 * 전달된 값 배열을 각 컬럼 폭에 맞게 정렬해
 * `| value | value |` 형태의 행 문자열을 출력 버퍼에 추가한다.
 * 헤더도 같은 형식을 쓰므로 별도 함수로 나누지 않고 공용 처리한다.
 */
static int append_table_row(
    char **buffer,
    size_t *length,
    size_t *capacity,
    char **values,
    const size_t *widths,
    size_t column_count,
    char *error,
    size_t error_size
) {
    size_t index = 0;

    if (!append_text(buffer, length, capacity, "|", error, error_size)) {
        return 0;
    }

    for (index = 0; index < column_count; ++index) {
        if (!append_text(buffer, length, capacity, " ", error, error_size)) {
            return 0;
        }

        if (!append_padded_value(buffer, length, capacity, values[index], widths[index], error, error_size)) {
            return 0;
        }

        if (!append_text(buffer, length, capacity, " |", error, error_size)) {
            return 0;
        }
    }

    return append_text(buffer, length, capacity, "\n", error, error_size);
}

/*
 * INSERT 명령을 실제 CSV 파일 append 작업으로 수행한다.
 *
 * 처리 순서는 다음과 같다.
 * 1. 테이블 파일 경로를 만든다.
 * 2. 헤더를 읽어 실제 컬럼 순서를 확인한다.
 * 3. SQL에서 받은 컬럼/값을 헤더 순서에 맞게 재배치한다.
 * 4. 파일 끝 상태를 확인한 뒤 새 행을 append한다.
 *
 * 이 함수는 "컬럼 이름 검증 + 값 검증 + 파일 쓰기"를 한 번에 담당하는
 * Storage 계층의 INSERT 구현체다.
 */
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

/*
 * SELECT 명령을 실행해 결과를 표 형태 문자열로 만든다.
 *
 * 처리 순서는 다음과 같다.
 * 1. 헤더를 읽고 요청한 컬럼이 실제 파일에 존재하는지 확인한다.
 * 2. 각 데이터 행에서 요청 컬럼만 추출해 메모리에 저장한다.
 * 3. 가장 긴 값 기준으로 컬럼 폭을 계산한다.
 * 4. 경계선/헤더/데이터 행을 조합해 최종 출력 문자열을 만든다.
 *
 * 최종 결과는 호출자가 free해야 하는 동적 문자열로 반환된다.
 */
int storage_select_rows(const SqlCommand *command, char **output, char *error, size_t error_size) {
    char *path = NULL;
    FILE *file = NULL;
    char *header_line = NULL;
    char *row_line = NULL;
    char **header_columns = NULL;
    char **row_columns = NULL;
    char ***selected_rows = NULL;
    int *requested_indices = NULL;
    size_t *column_widths = NULL;
    size_t header_count = 0;
    size_t row_count = 0;
    size_t selected_row_count = 0;
    size_t requested_index = 0;
    size_t row_index = 0;
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

    column_widths = (size_t *) malloc(sizeof(size_t) * command->column_count);
    if (column_widths == NULL) {
        set_error(error, error_size, "Out of memory while sizing SELECT output.");
        goto cleanup;
    }

    for (requested_index = 0; requested_index < command->column_count; ++requested_index) {
        requested_indices[requested_index] = find_column_index(header_columns, header_count, command->columns[requested_index]);
        if (requested_indices[requested_index] < 0) {
            set_error(error, error_size, "Requested column does not exist in table.");
            goto cleanup;
        }
        column_widths[requested_index] = strlen(command->columns[requested_index]);
    }

    while ((row_line = read_line_alloc(file)) != NULL) {
        char **selected_row = NULL;
        char ***grown_rows = NULL;

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

        selected_row = (char **) malloc(sizeof(char *) * command->column_count);
        if (selected_row == NULL) {
            set_error(error, error_size, "Out of memory while storing SELECT row.");
            goto cleanup;
        }

        for (requested_index = 0; requested_index < command->column_count; ++requested_index) {
            selected_row[requested_index] = NULL;
        }

        for (requested_index = 0; requested_index < command->column_count; ++requested_index) {
            size_t value_length = 0;

            selected_row[requested_index] = duplicate_string(row_columns[requested_indices[requested_index]]);
            if (selected_row[requested_index] == NULL) {
                set_error(error, error_size, "Out of memory while copying SELECT value.");
                free_string_list(selected_row, command->column_count);
                goto cleanup;
            }

            value_length = strlen(selected_row[requested_index]);
            if (value_length > column_widths[requested_index]) {
                column_widths[requested_index] = value_length;
            }
        }

        grown_rows = (char ***) realloc(selected_rows, sizeof(char **) * (selected_row_count + 1));
        if (grown_rows == NULL) {
            set_error(error, error_size, "Out of memory while growing SELECT rows.");
            free_string_list(selected_row, command->column_count);
            goto cleanup;
        }

        selected_rows = grown_rows;
        selected_rows[selected_row_count++] = selected_row;

        free(row_line);
        row_line = NULL;
        free_string_list(row_columns, row_count);
        row_columns = NULL;
        row_count = 0;
    }

    if (!append_table_border(output, &output_length, &output_capacity, column_widths, command->column_count, error, error_size)) {
        goto cleanup;
    }

    if (!append_table_row(output, &output_length, &output_capacity, command->columns, column_widths, command->column_count, error, error_size)) {
        goto cleanup;
    }

    if (!append_table_border(output, &output_length, &output_capacity, column_widths, command->column_count, error, error_size)) {
        goto cleanup;
    }

    for (row_index = 0; row_index < selected_row_count; ++row_index) {
        if (!append_table_row(output, &output_length, &output_capacity, selected_rows[row_index], column_widths, command->column_count, error, error_size)) {
            goto cleanup;
        }
    }

    if (!append_table_border(output, &output_length, &output_capacity, column_widths, command->column_count, error, error_size)) {
        goto cleanup;
    }

    ok = 1;

cleanup:
    free(path);
    free(header_line);
    free_string_list(header_columns, header_count);
    free(requested_indices);
    free(column_widths);
    free(row_line);
    free_string_list(row_columns, row_count);
    if (selected_rows != NULL) {
        for (row_index = 0; row_index < selected_row_count; ++row_index) {
            free_string_list(selected_rows[row_index], command->column_count);
        }
        free(selected_rows);
    }

    if (!ok) {
        free(*output);
        *output = NULL;
    }

    if (file != NULL) {
        fclose(file);
    }

    return ok;
}
