#include "cli_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_helpers.h"

static void test_cli_insert_and_select(void) {
    char workspace[1024];
    char original_directory[1024];
    char table_path[1024];
    FILE *stdout_file = NULL;
    FILE *stderr_file = NULL;
    char *stdout_contents = NULL;
    char *stderr_contents = NULL;
    char *table_contents = NULL;
    char *argv_insert[] = { "sql_processor", "insert.sql" };
    char *argv_select[] = { "sql_processor", "select.sql" };

    test_prepare_workspace("cli_case", workspace, sizeof(workspace));

    test_join_path(table_path, sizeof(table_path), workspace, "data");
    test_join_path(table_path, sizeof(table_path), table_path, "users.csv");
    test_write_file(table_path, "id,name\n");

    {
        char insert_path[1024];
        char select_path[1024];
        char stdout_path[1024];
        char stderr_path[1024];

        test_join_path(insert_path, sizeof(insert_path), workspace, "insert.sql");
        test_join_path(select_path, sizeof(select_path), workspace, "select.sql");
        test_join_path(stdout_path, sizeof(stdout_path), workspace, "stdout.txt");
        test_join_path(stderr_path, sizeof(stderr_path), workspace, "stderr.txt");

        test_write_file(insert_path, "INSERT INTO users (id, name) VALUES (1, 'jungle');");
        test_write_file(select_path, "SELECT id, name FROM users;");
        test_write_file(stdout_path, "");
        test_write_file(stderr_path, "");
    }

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    stdout_file = fopen("stdout.txt", "wb+");
    stderr_file = fopen("stderr.txt", "wb+");
    ASSERT_TRUE(stdout_file != NULL);
    ASSERT_TRUE(stderr_file != NULL);
    ASSERT_EQ_INT(0, run_cli_with_streams(2, argv_insert, stdout_file, stderr_file));
    fclose(stdout_file);
    fclose(stderr_file);

    stdout_contents = test_read_file("stdout.txt");
    stderr_contents = test_read_file("stderr.txt");
    table_contents = test_read_file("data/users.csv");
    ASSERT_STREQ("INSERT OK\n", stdout_contents);
    ASSERT_STREQ("", stderr_contents);
    ASSERT_STREQ("id,name\n1,jungle\n", table_contents);

    free(stdout_contents);
    free(stderr_contents);
    free(table_contents);

    test_write_file("stdout.txt", "");
    test_write_file("stderr.txt", "");

    stdout_file = fopen("stdout.txt", "wb+");
    stderr_file = fopen("stderr.txt", "wb+");
    ASSERT_TRUE(stdout_file != NULL);
    ASSERT_TRUE(stderr_file != NULL);
    ASSERT_EQ_INT(0, run_cli_with_streams(2, argv_select, stdout_file, stderr_file));
    fclose(stdout_file);
    fclose(stderr_file);

    stdout_contents = test_read_file("stdout.txt");
    stderr_contents = test_read_file("stderr.txt");
    ASSERT_STREQ(
        "+----+--------+\n"
        "| id | name   |\n"
        "+----+--------+\n"
        "| 1  | jungle |\n"
        "+----+--------+\n",
        stdout_contents
    );
    ASSERT_STREQ("", stderr_contents);

    free(stdout_contents);
    free(stderr_contents);
    test_leave_directory(original_directory);
}

static void test_cli_bad_sql_reports_error(void) {
    char workspace[1024];
    char original_directory[1024];
    FILE *stdout_file = NULL;
    FILE *stderr_file = NULL;
    char *stdout_contents = NULL;
    char *stderr_contents = NULL;
    char *argv_bad[] = { "sql_processor", "bad.sql" };

    test_prepare_workspace("cli_bad_sql", workspace, sizeof(workspace));

    {
        char bad_sql_path[1024];
        char stdout_path[1024];
        char stderr_path[1024];

        test_join_path(bad_sql_path, sizeof(bad_sql_path), workspace, "bad.sql");
        test_join_path(stdout_path, sizeof(stdout_path), workspace, "stdout.txt");
        test_join_path(stderr_path, sizeof(stderr_path), workspace, "stderr.txt");

        test_write_file(bad_sql_path, "SELECT id FROM users");
        test_write_file(stdout_path, "");
        test_write_file(stderr_path, "");
    }

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    stdout_file = fopen("stdout.txt", "wb+");
    stderr_file = fopen("stderr.txt", "wb+");
    ASSERT_TRUE(stdout_file != NULL);
    ASSERT_TRUE(stderr_file != NULL);
    ASSERT_EQ_INT(1, run_cli_with_streams(2, argv_bad, stdout_file, stderr_file));
    fclose(stdout_file);
    fclose(stderr_file);

    stdout_contents = test_read_file("stdout.txt");
    stderr_contents = test_read_file("stderr.txt");
    ASSERT_STREQ("", stdout_contents);
    ASSERT_TRUE(strstr(stderr_contents, "Statement must end with ';'") != NULL);

    free(stdout_contents);
    free(stderr_contents);
    test_leave_directory(original_directory);
}

static void test_cli_missing_input_file_reports_error(void) {
    char workspace[1024];
    char original_directory[1024];
    FILE *stdout_file = NULL;
    FILE *stderr_file = NULL;
    char *stdout_contents = NULL;
    char *stderr_contents = NULL;
    char *argv_missing[] = { "sql_processor", "missing.sql" };

    test_prepare_workspace("cli_missing_input", workspace, sizeof(workspace));

    {
        char stdout_path[1024];
        char stderr_path[1024];

        test_join_path(stdout_path, sizeof(stdout_path), workspace, "stdout.txt");
        test_join_path(stderr_path, sizeof(stderr_path), workspace, "stderr.txt");
        test_write_file(stdout_path, "");
        test_write_file(stderr_path, "");
    }

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    stdout_file = fopen("stdout.txt", "wb+");
    stderr_file = fopen("stderr.txt", "wb+");
    ASSERT_TRUE(stdout_file != NULL);
    ASSERT_TRUE(stderr_file != NULL);
    ASSERT_EQ_INT(1, run_cli_with_streams(2, argv_missing, stdout_file, stderr_file));
    fclose(stdout_file);
    fclose(stderr_file);

    stdout_contents = test_read_file("stdout.txt");
    stderr_contents = test_read_file("stderr.txt");
    ASSERT_STREQ("", stdout_contents);
    ASSERT_TRUE(strstr(stderr_contents, "Failed to open SQL input file.") != NULL);

    free(stdout_contents);
    free(stderr_contents);
    test_leave_directory(original_directory);
}

static void test_cli_empty_sql_reports_error(void) {
    char workspace[1024];
    char original_directory[1024];
    FILE *stdout_file = NULL;
    FILE *stderr_file = NULL;
    char *stdout_contents = NULL;
    char *stderr_contents = NULL;
    char *argv_empty[] = { "sql_processor", "empty.sql" };

    test_prepare_workspace("cli_empty_sql", workspace, sizeof(workspace));

    {
        char empty_path[1024];
        char stdout_path[1024];
        char stderr_path[1024];

        test_join_path(empty_path, sizeof(empty_path), workspace, "empty.sql");
        test_join_path(stdout_path, sizeof(stdout_path), workspace, "stdout.txt");
        test_join_path(stderr_path, sizeof(stderr_path), workspace, "stderr.txt");

        test_write_file(empty_path, "");
        test_write_file(stdout_path, "");
        test_write_file(stderr_path, "");
    }

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    stdout_file = fopen("stdout.txt", "wb+");
    stderr_file = fopen("stderr.txt", "wb+");
    ASSERT_TRUE(stdout_file != NULL);
    ASSERT_TRUE(stderr_file != NULL);
    ASSERT_EQ_INT(1, run_cli_with_streams(2, argv_empty, stdout_file, stderr_file));
    fclose(stdout_file);
    fclose(stderr_file);

    stdout_contents = test_read_file("stdout.txt");
    stderr_contents = test_read_file("stderr.txt");
    ASSERT_STREQ("", stdout_contents);
    ASSERT_TRUE(strstr(stderr_contents, "SQL input is empty.") != NULL);

    free(stdout_contents);
    free(stderr_contents);
    test_leave_directory(original_directory);
}

static void test_cli_whitespace_sql_reports_error(void) {
    char workspace[1024];
    char original_directory[1024];
    FILE *stdout_file = NULL;
    FILE *stderr_file = NULL;
    char *stdout_contents = NULL;
    char *stderr_contents = NULL;
    char *argv_blank[] = { "sql_processor", "blank.sql" };

    test_prepare_workspace("cli_blank_sql", workspace, sizeof(workspace));

    {
        char blank_path[1024];
        char stdout_path[1024];
        char stderr_path[1024];

        test_join_path(blank_path, sizeof(blank_path), workspace, "blank.sql");
        test_join_path(stdout_path, sizeof(stdout_path), workspace, "stdout.txt");
        test_join_path(stderr_path, sizeof(stderr_path), workspace, "stderr.txt");

        test_write_file(blank_path, " \r\n\t ");
        test_write_file(stdout_path, "");
        test_write_file(stderr_path, "");
    }

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    stdout_file = fopen("stdout.txt", "wb+");
    stderr_file = fopen("stderr.txt", "wb+");
    ASSERT_TRUE(stdout_file != NULL);
    ASSERT_TRUE(stderr_file != NULL);
    ASSERT_EQ_INT(1, run_cli_with_streams(2, argv_blank, stdout_file, stderr_file));
    fclose(stdout_file);
    fclose(stderr_file);

    stdout_contents = test_read_file("stdout.txt");
    stderr_contents = test_read_file("stderr.txt");
    ASSERT_STREQ("", stdout_contents);
    ASSERT_TRUE(strstr(stderr_contents, "SQL input is empty.") != NULL);

    free(stdout_contents);
    free(stderr_contents);
    test_leave_directory(original_directory);
}

static void test_cli_interactive_insert_and_select(void) {
    char workspace[1024];
    char original_directory[1024];
    char table_path[1024];
    FILE *input_file = NULL;
    FILE *stdout_file = NULL;
    FILE *stderr_file = NULL;
    char *stdout_contents = NULL;
    char *stderr_contents = NULL;
    char *table_contents = NULL;

    test_prepare_workspace("cli_interactive", workspace, sizeof(workspace));

    test_join_path(table_path, sizeof(table_path), workspace, "data");
    test_join_path(table_path, sizeof(table_path), table_path, "users.csv");
    test_write_file(table_path, "id,name\n");

    {
        char input_path[1024];
        char stdout_path[1024];
        char stderr_path[1024];

        test_join_path(input_path, sizeof(input_path), workspace, "interactive.txt");
        test_join_path(stdout_path, sizeof(stdout_path), workspace, "stdout.txt");
        test_join_path(stderr_path, sizeof(stderr_path), workspace, "stderr.txt");

        test_write_file(
            input_path,
            "INSERT INTO users (id, name) VALUES (2, 'sql');\n"
            "SELECT id, name FROM users;\n"
            "exit   \n"
        );
        test_write_file(stdout_path, "");
        test_write_file(stderr_path, "");
    }

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    input_file = fopen("interactive.txt", "rb");
    stdout_file = fopen("stdout.txt", "wb+");
    stderr_file = fopen("stderr.txt", "wb+");
    ASSERT_TRUE(input_file != NULL);
    ASSERT_TRUE(stdout_file != NULL);
    ASSERT_TRUE(stderr_file != NULL);
    ASSERT_EQ_INT(0, run_cli_interactive_with_streams(input_file, stdout_file, stderr_file));
    fclose(input_file);
    fclose(stdout_file);
    fclose(stderr_file);

    stdout_contents = test_read_file("stdout.txt");
    stderr_contents = test_read_file("stderr.txt");
    table_contents = test_read_file("data/users.csv");

    ASSERT_TRUE(strstr(stdout_contents, "Interactive SQL mode. Type 'exit' to quit.\n") != NULL);
    ASSERT_TRUE(strstr(stdout_contents, "INSERT OK\n") != NULL);
    ASSERT_TRUE(strstr(stdout_contents, "| 2  | sql  |\n") != NULL);
    ASSERT_STREQ("", stderr_contents);
    ASSERT_STREQ("id,name\n2,sql\n", table_contents);

    free(stdout_contents);
    free(stderr_contents);
    free(table_contents);
    test_leave_directory(original_directory);
}

int main(void) {
    test_cli_insert_and_select();
    test_cli_bad_sql_reports_error();
    test_cli_missing_input_file_reports_error();
    test_cli_empty_sql_reports_error();
    test_cli_whitespace_sql_reports_error();
    test_cli_interactive_insert_and_select();
    printf("test_cli passed\n");
    return 0;
}
