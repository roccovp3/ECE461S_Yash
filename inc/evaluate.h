#ifndef EVALUATE_H
#define EVALUATE_H

#include <yash.h>

int evaluate_command_tokens(int start_global_tokens_index, int stop_global_tokens_index, int num_tok, int *psaved_stdin,
                            int *psaved_stdout, int *psaved_stderr, char **prog_argv, int *pprog_argc,
                            process_stack_t *pprocess_stack);

#endif