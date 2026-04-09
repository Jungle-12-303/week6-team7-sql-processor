#include "parser.h"

#include <stdio.h>
#include <string.h>

#include "../test_helpers.h"

static void test_parse_insert(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(parse_sql("INSERT INTO users (id, name) VALUES (1, 'jungle');", &command, error, sizeof(error)));
    ASSERT_EQ_INT(SQL_COMMAND_INSERT, command.type);
    ASSERT_STREQ("users", command.table_name);
    ASSERT_EQ_INT(2, (int) command.column_count);
    ASSERT_STREQ("id", command.columns[0]);
    ASSERT_STREQ("name", command.columns[1]);
    ASSERT_EQ_INT(2, (int) command.value_count);
    ASSERT_STREQ("1", command.values[0]);
    ASSERT_STREQ("jungle", command.values[1]);
    sql_command_free(&command);
}

static void test_parse_select(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(parse_sql("SELECT id, name FROM users;", &command, error, sizeof(error)));
    ASSERT_EQ_INT(SQL_COMMAND_SELECT, command.type);
    ASSERT_STREQ("users", command.table_name);
    ASSERT_EQ_INT(2, (int) command.column_count);
    ASSERT_STREQ("id", command.columns[0]);
    ASSERT_STREQ("name", command.columns[1]);
    ASSERT_EQ_INT(0, (int) command.value_count);
    ASSERT_EQ_INT(0, command.has_where);
    sql_command_free(&command);
}

static void test_parse_select_with_where(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(parse_sql("SELECT id, name FROM users WHERE name = 'jungle';", &command, error, sizeof(error)));
    ASSERT_EQ_INT(SQL_COMMAND_SELECT, command.type);
    ASSERT_STREQ("users", command.table_name);
    ASSERT_EQ_INT(2, (int) command.column_count);
    ASSERT_STREQ("id", command.columns[0]);
    ASSERT_STREQ("name", command.columns[1]);
    ASSERT_EQ_INT(1, command.has_where);
    ASSERT_STREQ("name", command.where_column);
    ASSERT_STREQ("jungle", command.where_value);
    sql_command_free(&command);
}

static void test_missing_semicolon_fails(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(!parse_sql("SELECT id FROM users", &command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "Statement must end with ';'") != NULL);
    sql_command_free(&command);
}

static void test_empty_sql_fails(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(!parse_sql("", &command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "SQL input is empty.") != NULL);
    sql_command_free(&command);
}

static void test_whitespace_sql_fails(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(!parse_sql("   \r\n\t  ", &command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "SQL input is empty.") != NULL);
    sql_command_free(&command);
}

static void test_unsupported_statement_fails(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(!parse_sql("DELETE FROM users;", &command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "Unsupported SQL statement.") != NULL);
    sql_command_free(&command);
}

static void test_insert_column_value_mismatch_fails(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(!parse_sql("INSERT INTO users (id, name) VALUES (1);", &command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "Column count and value count must match.") != NULL);
    sql_command_free(&command);
}

static void test_invalid_where_fails(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(!parse_sql("SELECT id FROM users WHERE name 'jungle';", &command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "Expected '=' in WHERE clause.") != NULL);
    sql_command_free(&command);
}

static void test_select_star_fails(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(!parse_sql("SELECT * FROM users;", &command, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "SELECT * is not supported.") != NULL);
    sql_command_free(&command);
}

static void test_mixed_case_keywords_parse(void) {
    SqlCommand command;
    char error[256];

    sql_command_init(&command);
    ASSERT_TRUE(parse_sql("iNsErT InTo users (id, name) VaLuEs (1, 'jungle');", &command, error, sizeof(error)));
    ASSERT_EQ_INT(SQL_COMMAND_INSERT, command.type);
    ASSERT_STREQ("users", command.table_name);
    ASSERT_EQ_INT(2, (int) command.column_count);
    ASSERT_EQ_INT(2, (int) command.value_count);
    sql_command_free(&command);
}

int main(void) {
    test_parse_insert();
    test_parse_select();
    test_parse_select_with_where();
    test_missing_semicolon_fails();
    test_empty_sql_fails();
    test_whitespace_sql_fails();
    test_unsupported_statement_fails();
    test_insert_column_value_mismatch_fails();
    test_invalid_where_fails();
    test_select_star_fails();
    test_mixed_case_keywords_parse();
    printf("test_parser passed\n");
    return 0;
}
