#ifndef CLI_RUNNER_H
#define CLI_RUNNER_H

#include <stdio.h>

int run_cli(int argc, char **argv);
int run_cli_with_streams(int argc, char **argv, FILE *out_stream, FILE *err_stream);
int run_cli_interactive_with_streams(FILE *input_stream, FILE *out_stream, FILE *err_stream);

#endif
