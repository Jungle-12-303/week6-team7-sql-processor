#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define TEST_CHDIR _chdir
#define TEST_GETCWD _getcwd
#define TEST_MKDIR(path) _mkdir(path)
#define TEST_PATH_SEP "\\"
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define TEST_CHDIR chdir
#define TEST_GETCWD getcwd
#define TEST_MKDIR(path) mkdir(path, 0777)
#define TEST_PATH_SEP "/"
#endif

/*
 * 테스트 실패 시 즉시 중단하는 가장 기본적인 조건 검사 매크로다.
 *
 * 조건식이 거짓이면 파일명, 줄 번호, 실패한 표현식을 함께 출력해
 * 어느 테스트의 어떤 검증에서 실패했는지 바로 알 수 있게 해 준다.
 */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(1); \
        } \
    } while (0)

/*
 * 정수 값 두 개가 같은지 비교하는 테스트 전용 매크로다.
 *
 * expected와 actual을 한 번씩만 평가하고,
 * 값이 다르면 기대값과 실제값을 함께 출력한다.
 */
#define ASSERT_EQ_INT(expected, actual) \
    do { \
        int expected_value = (expected); \
        int actual_value = (actual); \
        if (expected_value != actual_value) { \
            fprintf(stderr, "Assertion failed at %s:%d: expected %d but got %d\n", __FILE__, __LINE__, expected_value, actual_value); \
            exit(1); \
        } \
    } while (0)

/*
 * 두 문자열이 완전히 같은지 비교하는 테스트 전용 매크로다.
 *
 * 주로 파싱 결과, 출력 문자열, 파일 내용 비교에 사용한다.
 * 다르면 기대 문자열과 실제 문자열을 함께 보여 준다.
 */
#define ASSERT_STREQ(expected, actual) \
    do { \
        const char *expected_value = (expected); \
        const char *actual_value = (actual); \
        if (strcmp(expected_value, actual_value) != 0) { \
            fprintf(stderr, "Assertion failed at %s:%d\nExpected: %s\nActual: %s\n", __FILE__, __LINE__, expected_value, actual_value); \
            exit(1); \
        } \
    } while (0)

/*
 * 테스트 코드에서 사용할 문자열 복제 함수다.
 *
 * 표준 `strdup`은 환경에 따라 경고나 호환성 이슈가 있을 수 있어
 * 테스트 내부에서 직접 동일한 동작을 구현해 사용한다.
 */
static char *test_strdup(const char *value) {
    size_t length = strlen(value);
    char *copy = (char *) malloc(length + 1);
    if (copy == NULL) {
        fprintf(stderr, "Out of memory in test_strdup.\n");
        exit(1);
    }

    memcpy(copy, value, length + 1);
    return copy;
}

/*
 * 테스트용 디렉터리가 없으면 생성한다.
 *
 * `tests/tmp/...` 아래에 워크스페이스를 만들 때 사용하며,
 * 이미 존재하는 디렉터리라면 오류 없이 그대로 둔다.
 */
static void test_ensure_directory(const char *path) {
    if (TEST_MKDIR(path) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create directory: %s\n", path);
        exit(1);
    }
}

/*
 * 테스트 입력 파일이나 기대 결과 파일을 빠르게 작성한다.
 *
 * CSV, SQL 입력, 임시 텍스트 파일을 준비할 때 사용한다.
 * content가 NULL이면 빈 파일만 생성한다.
 */
static void test_write_file(const char *path, const char *content) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "Failed to write file: %s\n", path);
        exit(1);
    }

    if (content != NULL) {
        fwrite(content, 1, strlen(content), file);
    }

    fclose(file);
}

/*
 * 파일 전체 내용을 메모리 문자열로 읽어 온다.
 *
 * CLI 출력 파일이나 Storage 결과 파일을 통째로 비교할 때 사용한다.
 * 반환된 문자열은 호출한 테스트가 free해야 한다.
 */
static char *test_read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    long length = 0;
    char *buffer = NULL;

    if (file == NULL) {
        fprintf(stderr, "Failed to read file: %s\n", path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer = (char *) malloc((size_t) length + 1);
    if (buffer == NULL) {
        fclose(file);
        fprintf(stderr, "Out of memory while reading file: %s\n", path);
        exit(1);
    }

    if (length > 0) {
        fread(buffer, 1, (size_t) length, file);
    }
    buffer[length] = '\0';

    fclose(file);
    return buffer;
}

/*
 * 운영체제에 맞는 경로 구분자를 사용해 두 경로 조각을 합친다.
 *
 * Windows와 POSIX 계열이 서로 다른 구분자를 쓰므로,
 * 테스트 코드에서 경로 조합 로직을 한 곳으로 모아 재사용한다.
 */
static void test_join_path(char *buffer, size_t buffer_size, const char *left, const char *right) {
    char temp[2048];
    snprintf(temp, sizeof(temp), "%s%s%s", left, TEST_PATH_SEP, right);
    snprintf(buffer, buffer_size, "%s", temp);
}

/*
 * 각 테스트가 사용할 독립적인 임시 작업 공간을 만든다.
 *
 * 기본 흐름은 다음과 같다.
 * 1. 현재 작업 디렉터리를 기준으로 `tests/tmp`를 만든다.
 * 2. 그 아래에 테스트별 전용 디렉터리를 만든다.
 * 3. Storage 계층이 기대하는 `data/` 디렉터리도 함께 만든다.
 *
 * 이렇게 하면 테스트끼리 같은 파일을 공유하지 않아 서로 영향을 덜 준다.
 */
static void test_prepare_workspace(const char *workspace_name, char *workspace_path, size_t workspace_size) {
    char current_directory[1024];
    char temp_root[1024];
    char data_path[1024];

#ifdef _WIN32
    if (TEST_GETCWD(current_directory, (int) sizeof(current_directory)) == NULL) {
#else
    if (TEST_GETCWD(current_directory, sizeof(current_directory)) == NULL) {
#endif
        fprintf(stderr, "Failed to get current working directory.\n");
        exit(1);
    }

    test_join_path(temp_root, sizeof(temp_root), current_directory, "tests");
    test_ensure_directory(temp_root);
    test_join_path(temp_root, sizeof(temp_root), temp_root, "tmp");
    test_ensure_directory(temp_root);
    test_join_path(workspace_path, workspace_size, temp_root, workspace_name);
    test_ensure_directory(workspace_path);
    test_join_path(data_path, sizeof(data_path), workspace_path, "data");
    test_ensure_directory(data_path);
}

/*
 * 테스트 실행 중 작업 디렉터리를 임시 워크스페이스로 바꾼다.
 *
 * 프로그램이 상대 경로 `data/...`를 사용하므로,
 * 테스트에서도 실제 실행과 비슷한 디렉터리 기준을 만들어 주기 위해 사용한다.
 * 바꾸기 전에 원래 위치를 저장해 두었다가 이후 복구에 쓴다.
 */
static void test_enter_directory(const char *path, char *original_path, size_t original_path_size) {
#ifdef _WIN32
    if (original_path_size > (size_t) INT_MAX) {
        fprintf(stderr, "Working directory buffer is too large for _getcwd.\n");
        exit(1);
    }

    if (TEST_GETCWD(original_path, (int) original_path_size) == NULL) {
#else
    if (TEST_GETCWD(original_path, original_path_size) == NULL) {
#endif
        fprintf(stderr, "Failed to capture current working directory.\n");
        exit(1);
    }

    if (TEST_CHDIR(path) != 0) {
        fprintf(stderr, "Failed to change directory to: %s\n", path);
        exit(1);
    }
}

/*
 * test_enter_directory로 변경한 작업 디렉터리를 원래 위치로 되돌린다.
 *
 * 테스트가 끝난 뒤 현재 프로세스의 작업 디렉터리가 계속 바뀐 채로 남아 있으면
 * 다음 테스트가 다른 상대 경로를 기준으로 동작할 수 있으므로 반드시 복구한다.
 */
static void test_leave_directory(const char *original_path) {
    if (TEST_CHDIR(original_path) != 0) {
        fprintf(stderr, "Failed to restore working directory to: %s\n", original_path);
        exit(1);
    }
}

#endif
