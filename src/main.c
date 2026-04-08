#include "cli_runner.h"

/*
 * 프로그램의 실제 시작점이다.
 *
 * main은 직접 SQL을 읽거나 해석하지 않는다.
 * 전달받은 명령행 인자를 그대로 CLI 실행 함수에 넘겨서
 * 파일 입력 모드 또는 interactive 모드가 시작되도록 연결만 담당한다.
 */
int main(int argc, char **argv) {
    return run_cli(argc, argv);
}
