#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stddef.h>

#include "command.h"

/*
 * 파싱된 명령을 실제 동작으로 연결하는 Executor 모듈의 공개 인터페이스다.
 *
 * Executor는 SQL 문자열을 직접 해석하지 않는다.
 * 대신 Parser가 만든 SqlCommand를 보고 INSERT인지 SELECT인지 분기하고,
 * Storage를 호출해 최종 결과 문자열을 만든다.
 */

/* 파싱된 명령 하나를 실행한다.
 *
 * command:
 *   Parser가 채운 명령 객체다.
 *
 * output:
 *   성공 시 사용자에게 보여줄 결과 문자열을 가리키게 된다.
 *   호출 측에서 사용 후 free 해야 한다.
 *
 * error / error_size:
 *   실패 시 사람이 읽을 수 있는 오류 메시지를 기록하는 버퍼다.
 *
 * 반환값:
 *   성공하면 1, 실패하면 0을 반환한다.
 */
int execute_command(const SqlCommand *command, char **output, char *error, size_t error_size);

#endif
