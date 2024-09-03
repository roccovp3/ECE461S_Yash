#include <yash.h>

extern char TOKENS[MAX_INPUT_SIZE][MAX_TOKEN_SIZE];

int parse_user_input(char *user_str, int *num_tok_before_pipe, int *bg)
{
    char *token;
    *bg = 0;

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
        if (!strcmp(token, "&"))
        {
            *bg = 1;
        }

        if (pipe_found && bg)
        {
            printf("YASH does not support | and & in the same command\n");
            return 1;
        }

        set_token(token, i);
        token = strtok(0, " ");
        i++;
    }

    if (!pipe_found)
    {
        *num_tok_before_pipe = 0;
    }

    return 0;
}
