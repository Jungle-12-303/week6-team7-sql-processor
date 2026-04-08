#include "executor.h"

#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 외부 버퍼에 안전하게 오류 메시지를 복사하는 공통 함수다.
 *
 * Executor 내부에서 실패 원인을 문자열로 남길 때 사용하며,
 * 버퍼가 없거나 크기가 0인 경우에는 아무 작업도 하지 않는다.
 */
static void set_error(char *error, size_t error_size, const char *message) {
    if (error == NULL || error_size == 0) {
        return;
    }

    snprintf(error, error_size, "%s", message);
}

/*
 * 결과 출력용 문자열을 새로 복제한다.
 *
 * INSERT 성공 시처럼 고정 문자열을 반환해야 할 때,
 * 호출 측에서 자유롭게 free 할 수 있도록 별도 메모리로 복사한다.
 */
static char *duplicate_string(const char *value) {
    size_t length = strlen(value);
    char *copy = (char *) malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, value, length + 1);
    return copy;
}

/*
 * Parser가 만든 SqlCommand를 실제 동작으로 연결한다.
 *
 * INSERT는 Storage에 append를 요청한 뒤 "INSERT OK" 메시지를 만들고,
 * SELECT는 Storage에 조회와 표 형식 결과 생성을 요청한다.
 * 즉, Executor는 SQL을 다시 해석하지 않고 "어떤 동작을 실행할지"만 결정한다.
 */
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
