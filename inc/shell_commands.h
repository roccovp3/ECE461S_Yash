#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

#include <yash.h>

// return 0 if input was not a shell command, 1 otherwise
int shell_builtin_commands(process_stack_t *pprocess_stack, int num_tok);

#endif