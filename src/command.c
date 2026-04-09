#include "command.h"

#include <stdlib.h>

/*
 * SqlCommand 구조체를 초기 상태로 되돌린다.
 *
 * Parser가 결과를 채우기 전에 안전한 빈 상태를 만들거나,
 * 메모리 해제 후 다시 재사용 가능한 상태를 만들 때 사용한다.
 */
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
    command->has_where = 0;
    command->where_column = NULL;
    command->where_value = NULL;
}

/*
 * SqlCommand 내부에 동적으로 할당된 문자열과 배열을 모두 해제한다.
 *
 * table_name, columns, values는 Parser 과정에서 malloc/realloc으로 생성될 수 있으므로,
 * 명령 사용이 끝난 뒤 반드시 이 함수로 정리해야 메모리 누수를 막을 수 있다.
 */
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
    free(command->where_column);
    free(command->where_value);

    sql_command_init(command);
}
