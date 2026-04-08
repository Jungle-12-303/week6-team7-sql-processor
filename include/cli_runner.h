#ifndef CLI_RUNNER_H
#define CLI_RUNNER_H

#include <stdio.h>

/*
 * CLI 실행 진입점 함수 선언 모음이다.
 *
 * 이 헤더는 프로그램을 어떤 입력 방식으로 실행할지 결정하는 함수들을 외부에 공개한다.
 * 실제 SQL 파싱이나 CSV 처리 로직은 구현하지 않고,
 * 입력을 받아 Parser/Executor 흐름으로 연결하는 역할만 담당한다.
 */

/* 일반 프로그램 실행 진입점이다.
 * argc, argv를 받아 파일 입력 모드 또는 interactive 모드로 분기한다.
 */
int run_cli(int argc, char **argv);

/* 표준 출력/표준 오류 스트림을 외부에서 주입받아 CLI를 실행한다.
 * 테스트 코드에서 stdout/stderr를 파일로 바꿔 검증할 때 사용한다.
 */
int run_cli_with_streams(int argc, char **argv, FILE *out_stream, FILE *err_stream);

/* interactive 모드 전용 실행 함수다.
 * 입력 스트림에서 한 줄씩 SQL을 읽고, 종료 명령이 들어올 때까지 반복 실행한다.
 */
int run_cli_interactive_with_streams(FILE *input_stream, FILE *out_stream, FILE *err_stream);

#endif
