#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 오류 버퍼에 메시지를 안전하게 기록하는 공통 함수다.
 *
 * Parser 내부 보조 함수들이 실패 원인을 일관된 방식으로 남기기 위해 사용한다.
 */
static void set_error(char *error, size_t error_size, const char *message) {
    if (error == NULL || error_size == 0) {
        return;
    }

    snprintf(error, error_size, "%s", message);
}

/*
 * [start, end) 구간의 문자열 일부를 새 메모리로 복사한다.
 *
 * Parser는 테이블명, 컬럼명, 값 등을 원본 SQL 문자열에서 잘라 내야 하므로,
 * 이 함수가 부분 문자열 생성의 기본 도구 역할을 한다.
 */
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

/*
 * 문자열 구간의 앞뒤 공백을 제거한다.
 *
 * SQL에서 값이나 문장 전체를 자를 때 공백까지 그대로 보관하면
 * 이후 비교와 파싱이 어려워지므로, 구간 포인터 자체를 이동시켜 정리한다.
 */
static void trim_span(const char **start, const char **end) {
    while (*start < *end && isspace((unsigned char) **start)) {
        (*start)++;
    }

    while (*end > *start && isspace((unsigned char) *(*end - 1))) {
        (*end)--;
    }
}

/*
 * 현재 커서가 가리키는 위치에서 연속된 공백을 모두 건너뛴다.
 *
 * 키워드, 식별자, 값 파싱 전에 반복적으로 호출되어
 * SQL의 일반적인 공백과 줄바꿈을 허용하도록 돕는다.
 */
static void skip_spaces(const char **cursor) {
    while (**cursor != '\0' && isspace((unsigned char) **cursor)) {
        (*cursor)++;
    }
}

/* 두 문자를 대소문자를 무시하고 비교한다. */
static int chars_equal_ignore_case(char left, char right) {
    return tolower((unsigned char) left) == tolower((unsigned char) right);
}

/*
 * 현재 위치가 특정 SQL 키워드로 시작하는지 검사한다.
 *
 * 단순 접두사 비교만 하지 않고, 뒤 문자가 식별자 일부인지도 확인해서
 * 예를 들어 SELECTX 같은 문자열을 SELECT로 오인하지 않도록 막는다.
 */
static int starts_with_keyword(const char *text, const char *keyword) {
    size_t index = 0;

    for (index = 0; keyword[index] != '\0'; ++index) {
        if (text[index] == '\0' || !chars_equal_ignore_case(text[index], keyword[index])) {
            return 0;
        }
    }

    return !(isalnum((unsigned char) text[index]) || text[index] == '_');
}

/*
 * 현재 커서에서 특정 키워드를 소비한다.
 *
 * 앞 공백을 넘긴 뒤 키워드가 맞으면 커서를 키워드 뒤로 이동시키고,
 * 아니면 아무 것도 소비하지 않은 채 실패를 반환한다.
 */
static int consume_keyword(const char **cursor, const char *keyword) {
    const char *probe = *cursor;

    skip_spaces(&probe);
    if (!starts_with_keyword(probe, keyword)) {
        return 0;
    }

    *cursor = probe + strlen(keyword);
    return 1;
}

/* SQL 식별자의 첫 글자로 허용되는 문자인지 검사한다. */
static int is_identifier_start(char value) {
    return isalpha((unsigned char) value) || value == '_';
}

/* SQL 식별자의 나머지 글자로 허용되는 문자인지 검사한다. */
static int is_identifier_part(char value) {
    return isalnum((unsigned char) value) || value == '_';
}

/*
 * 테이블명이나 컬럼명 같은 식별자 하나를 읽어 낸다.
 *
 * 현재 커서 위치에서 유효한 식별자 규칙을 만족하는지 검사하고,
 * 성공하면 새 문자열을 만들어 identifier에 돌려준다.
 */
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

/*
 * 동적 문자열 배열 뒤에 새 항목 하나를 추가한다.
 *
 * 컬럼 목록과 값 목록은 개수를 미리 알 수 없으므로,
 * Parser는 이 함수를 통해 필요한 만큼 배열을 확장한다.
 */
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

/*
 * 괄호 안의 식별자 목록을 파싱한다.
 *
 * INSERT의 컬럼 목록처럼 `(id, name)` 형태를 읽어서
 * 각 항목을 동적 배열에 순서대로 저장한다.
 */
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

/*
 * SELECT 절의 컬럼 목록을 파싱한다.
 *
 * 쉼표로 구분된 식별자를 읽다가 FROM 키워드를 만나면 종료한다.
 * 즉, SELECT와 FROM 사이 구간만 책임지고 해석한다.
 */
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

/*
 * 값 하나를 파싱한다.
 *
 * 작은따옴표로 감싼 문자열 값과, 쉼표/닫는 괄호 전까지의 일반 값을 모두 다룬다.
 * 현재 구현은 최소 문법만 지원하므로 복잡한 escape 규칙은 처리하지 않는다.
 */
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

/*
 * VALUES 절의 괄호 안 값 목록을 파싱한다.
 *
 * INSERT 문에서 `(1, 'jungle')` 같은 부분을 읽어
 * 값 배열에 순서대로 저장한다.
 */
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

/*
 * 현재 커서 뒤에 의미 있는 입력이 남아 있는지 검사한다.
 *
 * SQL 한 문장만 지원하므로, 남은 내용이 공백뿐인지 확인해서
 * 예상치 못한 뒤쪽 입력을 에러로 처리한다.
 */
static int ensure_end_of_input(const char **cursor, char *error, size_t error_size) {
    skip_spaces(cursor);
    if (**cursor != '\0') {
        set_error(error, error_size, "Unexpected trailing input.");
        return 0;
    }

    return 1;
}

/*
 * 원본 SQL 입력에서 실제로 파싱할 문장 본문만 추출한다.
 *
 * 빈 입력인지, 세미콜론이 있는지, 문장이 하나만 있는지 검증하고
 * 마지막 세미콜론을 제거한 뒤 앞뒤 공백까지 정리한 새 문자열을 만든다.
 */
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

/*
 * INSERT 문을 상세하게 파싱한다.
 *
 * INSERT -> INTO -> 테이블명 -> 컬럼 목록 -> VALUES -> 값 목록 순으로 읽고,
 * 마지막에 컬럼 수와 값 수가 일치하는지까지 확인한다.
 */
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

/*
 * SELECT 문을 상세하게 파싱한다.
 *
 * SELECT 뒤의 컬럼 목록과 FROM 뒤의 테이블명을 읽고,
 * 문장 끝에 불필요한 입력이 남아 있지 않은지 확인한다.
 */
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

/*
 * SQL 문자열 전체를 파싱해서 SqlCommand 구조체로 변환한다.
 *
 * 먼저 문장 추출과 기본 검증을 수행하고,
 * 시작 키워드가 INSERT인지 SELECT인지 판단한 뒤
 * 각각의 세부 파서로 넘긴다.
 */
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
