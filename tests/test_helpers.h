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

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(1); \
        } \
    } while (0)

#define ASSERT_EQ_INT(expected, actual) \
    do { \
        int expected_value = (expected); \
        int actual_value = (actual); \
        if (expected_value != actual_value) { \
            fprintf(stderr, "Assertion failed at %s:%d: expected %d but got %d\n", __FILE__, __LINE__, expected_value, actual_value); \
            exit(1); \
        } \
    } while (0)

#define ASSERT_STREQ(expected, actual) \
    do { \
        const char *expected_value = (expected); \
        const char *actual_value = (actual); \
        if (strcmp(expected_value, actual_value) != 0) { \
            fprintf(stderr, "Assertion failed at %s:%d\nExpected: %s\nActual: %s\n", __FILE__, __LINE__, expected_value, actual_value); \
            exit(1); \
        } \
    } while (0)

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

static void test_ensure_directory(const char *path) {
    if (TEST_MKDIR(path) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create directory: %s\n", path);
        exit(1);
    }
}

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

static void test_join_path(char *buffer, size_t buffer_size, const char *left, const char *right) {
    char temp[2048];
    snprintf(temp, sizeof(temp), "%s%s%s", left, TEST_PATH_SEP, right);
    snprintf(buffer, buffer_size, "%s", temp);
}

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

static void test_leave_directory(const char *original_path) {
    if (TEST_CHDIR(original_path) != 0) {
        fprintf(stderr, "Failed to restore working directory to: %s\n", original_path);
        exit(1);
    }
}

#endif
