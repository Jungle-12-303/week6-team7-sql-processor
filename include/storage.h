#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

#include "command.h"

/*
 * CSV 파일 기반 저장소 모듈의 공개 인터페이스다.
 *
 * Storage는 테이블을 data/<table>.csv 파일에 대응시켜 다룬다.
 * SQL 문법 자체는 알지 못하고,
 * 이미 해석된 SqlCommand를 바탕으로 파일을 읽거나 쓰는 역할만 맡는다.
 */

/* INSERT 명령을 받아 대상 CSV 파일 끝에 새 레코드를 추가한다.
 *
 * command:
 *   INSERT 타입의 SqlCommand여야 하며,
 *   테이블명, 컬럼 목록, 값 목록이 채워져 있어야 한다.
 *
 * error / error_size:
 *   실패 시 오류 메시지를 기록하는 버퍼다.
 *
 * 반환값:
 *   성공하면 1, 실패하면 0을 반환한다.
 */
int storage_append_row(const SqlCommand *command, char *error, size_t error_size);

/* SELECT 명령을 받아 대상 CSV 파일에서 필요한 컬럼만 읽어 결과 문자열을 만든다.
 *
 * output:
 *   성공 시 표 형식 결과 문자열을 가리키게 된다.
 *   호출 측에서 사용 후 free 해야 한다.
 *
 * error / error_size:
 *   실패 시 오류 메시지를 기록하는 버퍼다.
 *
 * 반환값:
 *   성공하면 1, 실패하면 0을 반환한다.
 */
int storage_select_rows(const SqlCommand *command, char **output, char *error, size_t error_size);

#endif
