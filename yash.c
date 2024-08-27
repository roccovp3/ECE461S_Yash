#include <readline/history.h>
#include <readline/readline.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG

#define MAX_INPUT_SIZE 2000
#define MAX_TOKEN_SIZE 30

int parse_user_input(char *user_str)
{
    char *token;
    char tokens[MAX_INPUT_SIZE][MAX_TOKEN_SIZE] = {0};

    token = strtok(user_str, " ");

    for (int i = 0; token != 0; i++)
    {
        strncpy(tokens[i], token, MAX_TOKEN_SIZE);
        token = strtok(0, " ");
    }

#ifdef DEBUG
    for (int j = 0; j < MAX_INPUT_SIZE; j++)
    {
        if (strcmp(tokens[j], ""))
            printf("%s\n", tokens[j]);
    }
#endif

    return 0;
}

int main(int argc, char **argv)
{
    // printf("%s", getenv("PATH"));
    // char *args[] = {"hello\n"};
    // execvp("printf", args);

    char *user_str;
    for (;;)
    {
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
    }

    return 0;
}