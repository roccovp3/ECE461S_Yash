#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//#define DEBUG

#define MAX_INPUT_SIZE 2000
#define MAX_TOKEN_SIZE 30

char TOKENS[MAX_INPUT_SIZE][MAX_TOKEN_SIZE] = {0};

extern char **environ;

int shell_term;
int shell_focused;
pid_t shell_pgid;

int parse_user_input(char *user_str)
{
    char *token;

    token = strtok(user_str, " ");

    int i = 0;
    while (token != 0)
    {
        strncpy(TOKENS[i], token, MAX_TOKEN_SIZE);
        token = strtok(0, " ");
        i++;
    }

#ifdef DEBUG
    for (int j = 0; j < MAX_INPUT_SIZE; j++)
    {
        if (strcmp(TOKENS[j], ""))
            printf("%s\n", TOKENS[j]);
    }
#endif
    return 0;
}

void init_shell()
{
    shell_term = STDIN_FILENO;
    shell_focused = isatty(shell_term);
    shell_pgid = getpgrp();

    if(shell_focused)
    {
        while(tcgetpgrp(shell_term) != shell_pgid)
        {
            kill(-shell_pgid, SIGTTIN);
            shell_pgid = getpgrp();
        }
        
        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);

        shell_pgid = getpid();
        
        if(setpgid(shell_pgid, shell_pgid) < 0)
        {
            perror("Couldn't put shell in its own process group");
            exit(1);
        }

        tcsetpgrp(shell_term, shell_pgid);

    }
}

void spawn_process(char *argv[], pid_t pgid, int infile, int outfile, int errfile, int fg)
{
    if(shell_focused)
    {
        pid_t pid = getpid();
        if(pgid == 0) pgid = pid;
        setpgid(pid, pgid);

        if(fg)
        {
            tcsetpgrp(shell_term, pgid);
        }

        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    }
    
    if (infile != STDIN_FILENO)
    {
        dup2 (infile, STDIN_FILENO);
        close (infile);
    }
    if (outfile != STDOUT_FILENO)
    {
        dup2 (outfile, STDOUT_FILENO);
        close (outfile);
    }
    if (errfile != STDERR_FILENO)
    {
        dup2 (errfile, STDERR_FILENO);
        close (errfile);
    }

    execvp(argv[0], argv);
    perror("Failed to execute process");
    exit(1);
}

int main(int argc, char **argv)
{

    char *user_str;

    for (;;)
    {
        int saved_stdin = STDIN_FILENO;
        int saved_stdout = STDOUT_FILENO;
        int saved_stderr = STDERR_FILENO;

        int my_pipe[2] = {0};

        char *jprog_argv[MAX_INPUT_SIZE];
        int jprog_argc = 0;

        char *kprog_argv[MAX_INPUT_SIZE];
        int kprog_argc = 0;

        memset(TOKENS, 0, MAX_INPUT_SIZE * MAX_TOKEN_SIZE);
        memset(jprog_argv, 0, MAX_INPUT_SIZE);
        memset(kprog_argv, 0, MAX_INPUT_SIZE);

        int status;
        user_str = readline("# ");
        parse_user_input(user_str);

        int num_tok_before_pipe = 0;
        if (strstr(user_str, "|"))
        {
            while (strcmp(TOKENS[num_tok_before_pipe], "|"))
            {
                num_tok_before_pipe++;
            }
        }

        free(user_str);

        int num_tok = 0;
        while (strcmp(TOKENS[num_tok], ""))
        {
            num_tok++;
        }

        for (int j = num_tok_before_pipe; j < num_tok; j++)
        {

            if (!strcmp(TOKENS[j], "<"))
            {
                // this check is separate and before the other to ensure no out of bounds access.
                if (j + 1 == num_tok)
                {
                    printf("Missing filename for \"<\"\n");
                    break;
                }
                if (TOKENS[j + 1] != NULL)
                {
                    saved_stdin = open(TOKENS[j + 1], O_RDWR|O_CREAT, 0666);
                    j++; // skip next token, we already know it should be the filename
                }
                else
                {
                    printf("Missing filename for \"<\"\n");
                }
            }
            else if (!strcmp(TOKENS[j], ">"))
            {
                // this check is separate and before the other to ensure no out of bounds access.
                if (j + 1 == num_tok)
                {
                    printf("Missing filename for \">\"\n");
                    break;
                }
                if (TOKENS[j + 1] != NULL)
                {
                    saved_stdout = open(TOKENS[j + 1], O_RDWR|O_CREAT, 0666);
                    j++; // skip next token, we already know it should be the filename
                }
                else
                {
                    printf("Missing filename for \">\"\n");
                }
            }
            else if (!strcmp(TOKENS[j], "2>"))
            {
                // this check is separate and before the other to ensure no out of bounds access.
                if (j + 1 == num_tok)
                {
                    printf("Missing filename for \"2>\"\n");
                    break;
                }
                if (TOKENS[j + 1] != NULL)
                {
                    saved_stderr = open(TOKENS[j + 1], O_RDWR|O_CREAT, 0666);
                    j++; // skip next token, we already know it should be the filename
                }
                else
                {
                    printf("Missing filename for \"2>\"\n");
                }
            }
            else if (!strcmp(TOKENS[j], "&"))
            {
                /* code */
            }
            else
            {
                jprog_argv[jprog_argc] = TOKENS[j];
                jprog_argc++;
            }
        }

        if ((0 < num_tok_before_pipe) && (num_tok_before_pipe < num_tok))
        {
            for (int k = 0; k < num_tok_before_pipe; k++)
            {
                if (!strcmp(TOKENS[k], "<"))
                {
                    // this check is separate and before the other to ensure no out of bounds access.
                    if (k + 1 == num_tok)
                    {
                        printf("Missing filename for \"<\"\n");
                        break;
                    }
                    if (TOKENS[k + 1] != NULL)
                    {
                        fclose(stdin);
                        stdin = fopen(TOKENS[k + 1], "w");
                        k++; // skip next token, we already know it should be the filename
                    }
                    else
                    {
                        printf("Missing filename for \"<\"\n");
                    }
                }
                else if (!strcmp(TOKENS[k], ">"))
                {
                    // this check is separate and before the other to ensure no out of bounds access.
                    if (k + 1 == num_tok)
                    {
                        printf("Missing filename for \">\"\n");
                        break;
                    }
                    if (TOKENS[k + 1] != NULL)
                    {
                        fclose(stdout);
                        stdout = fopen(TOKENS[k + 1], "w");
                        k++; // skip next token, we already know it should be the filename
                    }
                    else
                    {
                        printf("Missing filename for \">\"\n");
                    }
                }
                else if (!strcmp(TOKENS[k], "2>"))
                {
                    // this check is separate and before the other to ensure no out of bounds access.
                    if (k + 1 == num_tok)
                    {
                        printf("Missing filename for \"2>\"\n");
                        break;
                    }
                    if (TOKENS[k + 1] != NULL)
                    {
                        fclose(stderr);
                        stderr = fopen(TOKENS[k + 1], "w");
                        k++; // skip next token, we already know it should be the filename
                    }
                    else
                    {
                        printf("Missing filename for \"2>\"\n");
                    }
                }
                else if (!strcmp(TOKENS[k], "&"))
                {
                    /* code */
                }
                else
                {
                    kprog_argv[kprog_argc] = TOKENS[k];
                    kprog_argc++;
                }
            }
            pipe(my_pipe);
            pid_t pid = fork();
            if (pid == (pid_t)0)
            {
                close(my_pipe[1]);
                execvp(kprog_argv[0], kprog_argv);
                printf("here3");
            }
            else
            {
                close(my_pipe[0]);
                execvp(jprog_argv[0], jprog_argv);
                printf("here2");
            }
        }
        else
        {
            pid_t pid = fork();
            if (pid == (pid_t)0)
            {
                spawn_process(jprog_argv, pid, saved_stdin, saved_stdout, saved_stderr, 1);
            }
            else
            {
                // this is the parent (shell)
                if(shell_focused)
                {
                    if(!shell_pgid)
                    {
                        shell_pgid = pid;
                    }
                    setpgid(pid, shell_pgid);
                }
            }
        }

        while(wait(&status) > 0){}
    }

    return 0;
}