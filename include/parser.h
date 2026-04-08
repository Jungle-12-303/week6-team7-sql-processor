#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "command.h"

/*
 * SQL 문자열을 내부 명령 구조로 변환하는 Parser 모듈의 공개 인터페이스다.
 *
 * 현재 프로젝트에서는 최소 범위의 INSERT, SELECT만 지원한다.
 * Parser의 책임은 문자열을 해석해서 SqlCommand를 채우는 것까지이며,
 * 파일 접근이나 실제 데이터 저장/조회는 여기서 하지 않는다.
 */

/* SQL 문자열 하나를 파싱해서 SqlCommand 구조체에 채운다.
 *
 * sql:
 *   사용자가 입력한 SQL 문자열이다.
 *
 * command:
 *   파싱 성공 시 결과가 채워질 출력 구조체다.
 *
 * error / error_size:
 *   실패 시 오류 메시지를 기록하는 버퍼다.
 *
 * 반환값:
 *   성공하면 1, 실패하면 0을 반환한다.
 */
int parse_sql(const char *sql, SqlCommand *command, char *error, size_t error_size);

#endif
