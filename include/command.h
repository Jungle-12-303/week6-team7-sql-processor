#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

/*
 * Parser가 해석한 SQL 결과를 저장하는 공통 데이터 구조 선언부다.
 *
 * 이 헤더는 Parser, Executor, Storage, CLI가 함께 사용하는 핵심 타입을 정의한다.
 * 즉, "문자열 SQL"을 "프로그램이 이해할 수 있는 명령 객체"로 바꿨을 때
 * 그 결과를 어떤 형태로 표현할지에 대한 약속이라고 보면 된다.
 */

/* 현재 지원하는 SQL 명령 종류를 구분하는 열거형이다. */
typedef enum SqlCommandType {
    SQL_COMMAND_INSERT = 0,
    SQL_COMMAND_SELECT = 1
} SqlCommandType;

/* SQL 한 문장을 내부 표현으로 담는 구조체다.
 *
 * type:
 *   INSERT인지 SELECT인지 나타낸다.
 *
 * table_name:
 *   대상 테이블 이름이다. 예: users
 *
 * columns / column_count:
 *   SQL에 등장한 컬럼 목록과 개수다.
 *   INSERT와 SELECT 모두 사용한다.
 *
 * values / value_count:
 *   INSERT의 VALUES 절에 들어 있는 값 목록과 개수다.
 *   SELECT에서는 사용하지 않으므로 비어 있을 수 있다.
 *
 * has_where / where_column / where_value:
 *   현재는 SELECT에서만 사용하는 공통 조건 표현 필드다.
 *   WHERE 절이 있으면 has_where가 1이고, 비교할 컬럼과 값이 채워진다.
 */
typedef struct SqlCommand {
    SqlCommandType type;
    char *table_name;
    char **columns;
    size_t column_count;
    char **values;
    size_t value_count;
    int has_where;
    char *where_column;
    char *where_value;
} SqlCommand;

/* SqlCommand 구조체를 안전한 초기 상태로 만든다.
 * 포인터 필드를 NULL로, 개수 필드를 0으로 초기화한다.
 */
void sql_command_init(SqlCommand *command);

/* SqlCommand 내부에 동적으로 할당된 메모리를 모두 해제한다.
 * 해제 후에는 다시 초기 상태로 되돌린다.
 */
void sql_command_free(SqlCommand *command);

#endif
