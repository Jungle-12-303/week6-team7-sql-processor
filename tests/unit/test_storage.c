#include "command.h"
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_helpers.h"

static void test_append_and_select(void) {
    char workspace[1024];
    char original_directory[1024];
    char table_path[1024];
    char *file_contents = NULL;
    char *select_output = NULL;
    char error[256];
    SqlCommand insert_command;
    SqlCommand select_command;

    test_prepare_workspace("storage_case", workspace, sizeof(workspace));
    test_join_path(table_path, sizeof(table_path), workspace, "data");
    test_join_path(table_path, sizeof(table_path), table_path, "users.csv");
    test_write_file(table_path, "id,name\n1,seed\n");

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    sql_command_init(&insert_command);
    insert_command.type = SQL_COMMAND_INSERT;
    insert_command.table_name = test_strdup("users");
    insert_command.column_count = 2;
    insert_command.value_count = 2;
    insert_command.columns = (char **) malloc(sizeof(char *) * 2);
    insert_command.values = (char **) malloc(sizeof(char *) * 2);
    insert_command.columns[0] = test_strdup("name");
    insert_command.columns[1] = test_strdup("id");
    insert_command.values[0] = test_strdup("jungle");
    insert_command.values[1] = test_strdup("2");

    ASSERT_TRUE(storage_append_row(&insert_command, error, sizeof(error)));

    file_contents = test_read_file("data/users.csv");
    ASSERT_STREQ("id,name\n1,seed\n2,jungle\n", file_contents);
    free(file_contents);
    file_contents = NULL;

    sql_command_init(&select_command);
    select_command.type = SQL_COMMAND_SELECT;
    select_command.table_name = test_strdup("users");
    select_command.column_count = 2;
    select_command.columns = (char **) malloc(sizeof(char *) * 2);
    select_command.columns[0] = test_strdup("name");
    select_command.columns[1] = test_strdup("id");

    ASSERT_TRUE(storage_select_rows(&select_command, &select_output, error, sizeof(error)));
    ASSERT_STREQ(
        "+--------+----+\n"
        "| name   | id |\n"
        "+--------+----+\n"
        "| seed   | 1  |\n"
        "| jungle | 2  |\n"
        "+--------+----+\n",
        select_output
    );

    free(select_output);
    sql_command_free(&insert_command);
    sql_command_free(&select_command);
    test_leave_directory(original_directory);
}

static void test_select_missing_column_fails(void) {
    char workspace[1024];
    char original_directory[1024];
    char table_path[1024];
    char error[256];
    char *select_output = NULL;
    SqlCommand command;

    test_prepare_workspace("storage_missing_column", workspace, sizeof(workspace));
    test_join_path(table_path, sizeof(table_path), workspace, "data");
    test_join_path(table_path, sizeof(table_path), table_path, "users.csv");
    test_write_file(table_path, "id,name\n1,jungle\n");

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    sql_command_init(&command);
    command.type = SQL_COMMAND_SELECT;
    command.table_name = test_strdup("users");
    command.column_count = 1;
    command.columns = (char **) malloc(sizeof(char *));
    command.columns[0] = test_strdup("age");

    ASSERT_TRUE(!storage_select_rows(&command, &select_output, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "Requested column does not exist in table.") != NULL);

    sql_command_free(&command);
    test_leave_directory(original_directory);
}

static void test_missing_table_fails(void) {
    char workspace[1024];
    char original_directory[1024];
    char error[256];
    char *select_output = NULL;
    SqlCommand command;

    test_prepare_workspace("storage_missing_table", workspace, sizeof(workspace));
    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    sql_command_init(&command);
    command.type = SQL_COMMAND_SELECT;
    command.table_name = test_strdup("users");
    command.column_count = 1;
    command.columns = (char **) malloc(sizeof(char *));
    command.columns[0] = test_strdup("id");

    ASSERT_TRUE(!storage_select_rows(&command, &select_output, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "Target table file does not exist.") != NULL);

    sql_command_free(&command);
    test_leave_directory(original_directory);
}

static void test_insert_value_with_comma_fails(void) {
    char workspace[1024];
    char original_directory[1024];
    char table_path[1024];
    char error[256];
    SqlCommand command;

    test_prepare_workspace("storage_bad_value", workspace, sizeof(workspace));
    test_join_path(table_path, sizeof(table_path), workspace, "data");
    test_join_path(table_path, sizeof(table_path), table_path, "users.csv");
    test_write_file(table_path, "id,name\n");

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    sql_command_init(&command);
    command.type = SQL_COMMAND_INSERT;
    command.table_name = test_strdup("users");
    command.column_count = 2;
    command.value_count = 2;
    command.columns = (char **) malloc(sizeof(char *) * 2);
    command.values = (char **) malloc(sizeof(char *) * 2);
    command.columns[0] = test_strdup("id");
    command.columns[1] = test_strdup("name");
    command.values[0] = test_strdup("1");
    command.values[1] = test_strdup("jung,le");

    ASSERT_TRUE(!storage_append_row(&command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "Values cannot contain commas or newlines in minimal CSV mode.") != NULL);

    sql_command_free(&command);
    test_leave_directory(original_directory);
}

static void test_corrupted_csv_row_fails(void) {
    char workspace[1024];
    char original_directory[1024];
    char table_path[1024];
    char error[256];
    char *select_output = NULL;
    SqlCommand command;

    test_prepare_workspace("storage_corrupted_csv", workspace, sizeof(workspace));
    test_join_path(table_path, sizeof(table_path), workspace, "data");
    test_join_path(table_path, sizeof(table_path), table_path, "users.csv");
    test_write_file(table_path, "id,name\n1,jungle,extra\n");

    test_enter_directory(workspace, original_directory, sizeof(original_directory));

    sql_command_init(&command);
    command.type = SQL_COMMAND_SELECT;
    command.table_name = test_strdup("users");
    command.column_count = 2;
    command.columns = (char **) malloc(sizeof(char *) * 2);
    command.columns[0] = test_strdup("id");
    command.columns[1] = test_strdup("name");

    ASSERT_TRUE(!storage_select_rows(&command, &select_output, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "CSV row does not match header column count.") != NULL);

    sql_command_free(&command);
    test_leave_directory(original_directory);
}

int main(void) {
    test_append_and_select();
    test_select_missing_column_fails();
    test_missing_table_fails();
    test_insert_value_with_comma_fails();
    test_corrupted_csv_row_fails();
    printf("test_storage passed\n");
    return 0;
}
