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

// #define DEBUG

#define MAX_INPUT_SIZE 2000
#define MAX_TOKEN_SIZE 30

char TOKENS[MAX_INPUT_SIZE][MAX_TOKEN_SIZE] = {0};

typedef struct
{
    pid_t arr[1000]; // if you have more than 1000 processes in the background im sorry...
    char user_str[1000][MAX_INPUT_SIZE];
    int top;
} process_stack_t;

int at_prompt = 0;
int shell_terminal;
int shell_is_interactive;
pid_t shell_pgid;
struct termios shell_tmodes;

int parse_user_input(char *user_str, int *num_tok_before_pipe)
{
    char *token;

    token = strtok(user_str, " ");

    int i = 0;
    int pipe_found = 0;
    while (token != 0)
    {
        if (!strcmp(token, "|"))
        {
            pipe_found = 1;
            (*num_tok_before_pipe)++;
        }
        if (strcmp(token, "|") && !pipe_found)
        {
            (*num_tok_before_pipe)++;
        }

        strncpy(TOKENS[i], token, MAX_TOKEN_SIZE);
        token = strtok(0, " ");
        i++;
    }

    if (!pipe_found)
    {
        *num_tok_before_pipe = 0;
    }

    return 0;
}

void interrupt_handler()
{
    fflush(stdout);
    printf("\n");
    if (at_prompt)
    {
        printf("# ");
    }
}

void suspend_handler()
{
    fflush(stdout);
    printf("\n");
    if (at_prompt)
    {
        printf("# ");
    }
}

void quit_handler()
{
    exit(0);
}

void spawn_process(char *argv[], pid_t pgid, int infile, int outfile, int errfile, int fg)
{
    pid_t pid = getpid();
    if (pgid == 0)
        pgid = pid;
    setpgid(pid, pgid);
    if (fg)
        tcsetpgrp(shell_terminal, pgid);

    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGSTOP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    if (infile != STDIN_FILENO)
    {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if (outfile != STDOUT_FILENO)
    {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    if (errfile != STDERR_FILENO)
    {
        dup2(errfile, STDERR_FILENO);
        close(errfile);
    }
    execvp(argv[0], argv);
    perror("YASH: Failed to execute process\n");
    exit(1);
}

// return 0 if input was not a shell command, 1 otherwise
int shell_builtin_commands(process_stack_t *pprocess_stack, int num_tok)
{
    int status;
    if (!strcmp(TOKENS[0], "fg") && (num_tok == 1))
    {
        if ((pprocess_stack->top) == -1)
        {
            return 1;
        }
        while (kill(pprocess_stack->arr[pprocess_stack->top], 0) != 0)
        {
            printf("pid checking: %d", pprocess_stack->arr[pprocess_stack->top]);
            // process no longer exists
            (pprocess_stack->top)--;

            if ((pprocess_stack->top) == -1)
            {
                printf("No processes in the background\n");
                return 1;
            }
        }

        while (1)
        {
            // process no longer stopped
            if ((pprocess_stack->top) == -1)
            {
                printf("No process in the background\n");
                return 1;
            }
            if (kill((pprocess_stack->arr)[pprocess_stack->top], SIGCONT) == 0)
            {
                pprocess_stack->top--;
                tcsetpgrp(shell_terminal, pprocess_stack->arr[pprocess_stack->top + 1]);
                printf("%s\n", (pprocess_stack->user_str[pprocess_stack->top+1]));
                waitpid(pprocess_stack->arr[pprocess_stack->top + 1], &status, WUNTRACED);
                tcsetpgrp(shell_terminal, shell_pgid);
                if (WIFSTOPPED(status) && !WIFEXITED(status))
                {
                    pprocess_stack->top++;
                }
                tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                return 1;
            }
            pprocess_stack->top--;
        }

        printf("No processes in the background\n");
        return 1;
    }
    else if(!strcmp(TOKENS[0], "bg") && (num_tok == 1))
    {
        if ((pprocess_stack->top) == -1)
        {
            return 1;
        }
        while (kill(pprocess_stack->arr[pprocess_stack->top], 0) != 0)
        {
            printf("pid checking: %d", pprocess_stack->arr[pprocess_stack->top]);
            // process no longer exists
            (pprocess_stack->top)--;

            if ((pprocess_stack->top) == -1)
            {
                printf("No processes in the background\n");
                return 1;
            }
        }

        while (1)
        {
            // process no longer stopped
            if ((pprocess_stack->top) == -1)
            {
                printf("No process in the background\n");
                return 1;
            }
            if (kill((pprocess_stack->arr)[pprocess_stack->top], SIGCONT) == 0)
            {
                pprocess_stack->top--;
                pid_t pid = pprocess_stack->arr[pprocess_stack->top + 1];
                tcsetpgrp(shell_terminal, pid);
                printf("[%d]%c %s\t%s\n", pid, '+', "Running", (pprocess_stack->user_str[pprocess_stack->top+1]));
                waitpid(pid, &status, WNOHANG|WUNTRACED);
                tcsetpgrp(shell_terminal, shell_pgid);
                if (WIFSTOPPED(status) && !WIFEXITED(status))
                {
                    pprocess_stack->top++;
                }
                tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                return 1;
            }
            pprocess_stack->top--;
        }

        printf("No processes in the background\n");
        return 1;
    }
    else if(!strcmp(TOKENS[0], "jobs") && (num_tok == 1))
    {
        int pprocess_stack_top = pprocess_stack->top;
        if(pprocess_stack_top == -1) return 1;
        waitpid(pprocess_stack->arr[pprocess_stack_top], &status, WNOHANG|WUNTRACED);
        if(WIFSTOPPED(status))
        {
            printf("[%d]%c %s\t%s\n", pprocess_stack->arr[pprocess_stack_top], '+', "Stopped", (pprocess_stack->user_str[pprocess_stack_top]));
        }
        else
        {
            printf("[%d]%c %s\t%s\n", pprocess_stack->arr[pprocess_stack_top], '+', "Running", (pprocess_stack->user_str[pprocess_stack_top]));
        }
        pprocess_stack_top--;
        while(pprocess_stack_top >= 0)
        {
            waitpid(pprocess_stack->arr[pprocess_stack_top], &status, WNOHANG|WUNTRACED);
            if(WIFSTOPPED(status))
            {
                printf("[%d]%c %s\t%s\n", pprocess_stack->arr[pprocess_stack_top], '-', "Stopped", (pprocess_stack->user_str[pprocess_stack_top]));
            }
            else
            {
                printf("[%d]%c %s\t%s\n", pprocess_stack->arr[pprocess_stack_top], '-', "Running", (pprocess_stack->user_str[pprocess_stack_top]));
            }
            pprocess_stack_top--;
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

int evaluate_command_tokens(int start_global_tokens_index, int stop_global_tokens_index, int num_tok, int *psaved_stdin,
                            int *psaved_stdout, int *psaved_stderr, char **prog_argv, int *pprog_argc,
                            process_stack_t *pprocess_stack)
{
    int user_input_valid = 1;
    for (int j = start_global_tokens_index; j < stop_global_tokens_index; j++)
    {

        int shell_command = shell_builtin_commands(pprocess_stack, num_tok);
        if (shell_command)
            break;

        if (!strcmp(TOKENS[j], "<"))
        {
            // this check is separate and before the other to ensure no out of bounds access.
            if (j + 1 == num_tok)
            {
                printf("YASH: Missing filename for \"<\"\n");
                user_input_valid = 0;
                break;
            }
            if (TOKENS[j + 1] != NULL)
            {
                if (access(TOKENS[j + 1], F_OK) == 0)
                {
                    *psaved_stdin = open(TOKENS[j + 1], O_RDWR, 0666);
                    j++; // skip next token, we already know it should be the filename
                }
                else
                {
                    printf("YASH: STDIN file \"%s\" does not exist\n", TOKENS[j + 1]);
                    user_input_valid = 0;
                    break;
                }
            }
            else
            {
                printf("YASH: Missing filename for \"<\"\n");
                user_input_valid = 0;
                break;
            }
        }
        else if (!strcmp(TOKENS[j], ">"))
        {
            // this check is separate and before the other to ensure no out of bounds access.
            if (j + 1 == num_tok)
            {
                printf("YASH: Missing filename for \">\"\n");
                user_input_valid = 0;
                break;
            }
            if (TOKENS[j + 1] != NULL)
            {
                *psaved_stdout = open(TOKENS[j + 1], O_RDWR | O_CREAT, 0666);
                j++; // skip next token, we already know it should be the filename
            }
            else
            {
                printf("YASH: Missing filename for \">\"\n");
                user_input_valid = 0;
                break;
            }
        }
        else if (!strcmp(TOKENS[j], "2>"))
        {
            // this check is separate and before the other to ensure no out of bounds access.
            if (j + 1 == num_tok)
            {
                printf("YASH: Missing filename for \"2>\"\n");
                user_input_valid = 0;
                break;
            }
            if (TOKENS[j + 1] != NULL)
            {
                *psaved_stderr = open(TOKENS[j + 1], O_RDWR | O_CREAT, 0666);
                j++; // skip next token, we already know it should be the filename
            }
            else
            {
                printf("YASH: Missing filename for \"2>\"\n");
                user_input_valid = 0;
                break;
            }
        }
        else if (!strcmp(TOKENS[j], "&"))
        {
            /* code */
        }
        else
        {
            prog_argv[*pprog_argc] = TOKENS[j];
            (*pprog_argc)++;
        }
    }
    return user_input_valid;
}

int main(int argc, char **argv)
{
    char* user_str;

    process_stack_t process_stack = {
        .top = -1,
        .arr = {-1},
    };

    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
        kill(-shell_pgid, SIGTTIN);

    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGSTOP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);

    for (;;)
    {
        int jsaved_stdin = STDIN_FILENO;
        int jsaved_stdout = STDOUT_FILENO;
        int jsaved_stderr = STDERR_FILENO;

        int ksaved_stdin = STDIN_FILENO;
        int ksaved_stdout = STDOUT_FILENO;
        int ksaved_stderr = STDERR_FILENO;

        int my_pipe[2] = {0};

        char *jprog_argv[MAX_INPUT_SIZE];
        int jprog_argc = 0;

        char *kprog_argv[MAX_INPUT_SIZE];
        int kprog_argc = 0;

        memset(TOKENS, 0, MAX_INPUT_SIZE * MAX_TOKEN_SIZE);
        memset(jprog_argv, 0, MAX_INPUT_SIZE);
        memset(kprog_argv, 0, MAX_INPUT_SIZE);

        int status = 0;

        fflush(stdout);
        at_prompt = 1;
        user_str = readline("# ");
        if(user_str == NULL)
        {
            //Ctrl-D (EOF) encountered
            exit(0);
        }
        at_prompt = 0;

        int num_tok_before_pipe = 0;

        char user_str_deep_copy[MAX_INPUT_SIZE];
        strncpy(user_str_deep_copy, user_str, MAX_INPUT_SIZE);

        parse_user_input(user_str, &num_tok_before_pipe);

        int num_tok = 0;
        while (strcmp(TOKENS[num_tok], ""))
        {
            num_tok++;
        }

        int user_input_valid =
            evaluate_command_tokens(num_tok_before_pipe, num_tok, num_tok, &jsaved_stdin, &jsaved_stdout,
                                    &jsaved_stderr, jprog_argv, &jprog_argc, &process_stack);

        // printf("numbfp %d", num_tok_before_pipe);
        if ((0 < num_tok_before_pipe) && (num_tok_before_pipe < num_tok))
        {
            // printf("happy");
            // &= because both process need valid input
            user_input_valid &=
                evaluate_command_tokens(0, num_tok_before_pipe - 1, num_tok_before_pipe - 1, &ksaved_stdin,
                                        &ksaved_stdout, &ksaved_stderr, kprog_argv, &kprog_argc, &process_stack);

            if (user_input_valid)
            {
                pipe(my_pipe);
                pid_t pid = fork();
                if (pid == (pid_t)0)
                {
                    // k child
                    close(my_pipe[0]);
                    dup2(my_pipe[1], STDOUT_FILENO);
                    spawn_process(kprog_argv, pid, ksaved_stdin, my_pipe[1], ksaved_stderr, 1);
                }
                else
                {
                    // parent, spawning j child
                    pid_t pid = fork();
                    if (pid == (pid_t)0)
                    {
                        close(my_pipe[1]);
                        dup2(my_pipe[0], STDIN_FILENO);
                        spawn_process(jprog_argv, pid, my_pipe[0], jsaved_stdout, jsaved_stderr, 1);
                    }
                    else
                    {
                        // parent
                        close(my_pipe[0]);
                        close(my_pipe[1]);
                        tcsetpgrp(shell_terminal, pid);
                        waitpid(pid, &status, WUNTRACED);
                        tcsetpgrp(shell_terminal, shell_pgid);
                        if (WIFSTOPPED(status) && !WIFEXITED(status))
                        {
                            process_stack.top++;
                            process_stack.arr[process_stack.top] = pid;
                            strcpy(process_stack.user_str[process_stack.top], user_str_deep_copy);
                        }
                        tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                    }
                }
            }
            else
            {
                printf("YASH: Invalid input");
            }
        }
        else
        {
            if (user_input_valid)
            {
                pid_t pid = fork();
                if (pid == (pid_t)0)
                {
                    spawn_process(jprog_argv, pid, jsaved_stdin, jsaved_stdout, jsaved_stderr, 1);
                }
                else
                {
                    // this is the parent (shell)
                    tcsetpgrp(shell_terminal, pid);
                    waitpid(pid, &status, WUNTRACED);
                    tcsetpgrp(shell_terminal, shell_pgid);
                    if (WIFSTOPPED(status) && !WIFEXITED(status))
                    {
                        process_stack.top++;
                        process_stack.arr[process_stack.top] = pid;
                        strcpy(process_stack.user_str[process_stack.top], user_str_deep_copy);
                    }
                    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                }
            }
            else
            {
                printf("YASH: Invalid input");
            }
        }
        free(user_str);
    }

    return 0;
}
