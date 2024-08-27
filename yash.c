#include <readline/history.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#define DEBUG

#define MAX_INPUT_SIZE 2000
#define MAX_TOKEN_SIZE 30

char TOKENS[MAX_INPUT_SIZE][MAX_TOKEN_SIZE] = {0};

extern char **environ;

int parse_user_input(char *user_str)
{
    char* token;

    token = strtok(user_str, " ");

    int i = 0;
    while(token != 0)
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

int eval_user_input(char **TOKENS)
{
}

int main(int argc, char **argv)
{
    char *user_str;

    int my_pipe[2];

    char* jprog_argv[MAX_INPUT_SIZE];
    int jprog_argc = 0;

    char* kprog_argv[MAX_INPUT_SIZE];
    int kprog_argc = 0;



    for (;;)
    {
        memset(TOKENS, 0, MAX_INPUT_SIZE*MAX_TOKEN_SIZE);
        memset(jprog_argv, 0, MAX_INPUT_SIZE);
        memset(kprog_argv, 0, MAX_INPUT_SIZE);

        user_str = readline("# ");

        if (strlen(user_str) > MAX_INPUT_SIZE)
        {
            printf("Input character limit of %d exceeded. Try again.\n", MAX_INPUT_SIZE);
        }
        else
        {
            parse_user_input(user_str);
        }

        
        free(user_str);

        int num_tok_before_pipe = 0;
        if(strstr(user_str, "|"))
        {
            while (strcmp(TOKENS[num_tok_before_pipe], "|"))
            {
                num_tok_before_pipe++;
            }
        }

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
                    fclose(stdin);
                    stdin = fopen(TOKENS[j + 1], "w");
                    j++; //skip next token, we already know it should be the filename
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
                    fclose(stdout);
                    stdout = fopen(TOKENS[j + 1], "w");
                    j++; //skip next token, we already know it should be the filename
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
                    fclose(stderr);
                    stderr = fopen(TOKENS[j + 1], "w");
                    j++; //skip next token, we already know it should be the filename
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
                        k++; //skip next token, we already know it should be the filename
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
                        k++; //skip next token, we already know it should be the filename
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
                        k++; //skip next token, we already know it should be the filename
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
            if(pid == (pid_t)0)
            {
                close(my_pipe[1]);
                execvp(kprog_argv[0], kprog_argv);
            }
            else
            {
                close(my_pipe[0]);
                execvp(jprog_argv[0], jprog_argv);
            }
        }
        else
        {
            pid_t pid = fork();
            execvp(jprog_argv[0], jprog_argv);
        }
    }

    return 0;
}