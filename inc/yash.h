#ifndef YASH_H
#define YASH_H

#include <errno.h>
#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 2000
#define MAX_TOKEN_SIZE 30
#define PROCESS_STACK_DEPTH 1000

typedef enum
{
    NONE = 0,
    RUNNING,
    STOPPED,
    DONE,
} status_t;

typedef struct
{
    pid_t arr[PROCESS_STACK_DEPTH]; // if you have more than 1000 processes in the background im sorry...
    char user_str[PROCESS_STACK_DEPTH][MAX_INPUT_SIZE];
    status_t status[PROCESS_STACK_DEPTH];
    int size;
    int top;
} process_stack_t;

pid_t get_shell_pgid();

int get_shell_terminal();

char* get_token(int i);

void set_token(char* token, int i);

#endif