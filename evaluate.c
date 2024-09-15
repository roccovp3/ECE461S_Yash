#include <shell_commands.h>
#include <yash.h>

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

        if (!strcmp(get_token(j), "<"))
        {
            // this check is separate and before the other to ensure no out of bounds access.
            if (j + 1 == num_tok)
            {
                printf("yash: Missing filename for \"<\"\n");
                user_input_valid = 0;
                break;
            }
            if(j == 0)
            {
               user_input_valid = 0;
               break;
            }
            if (get_token(j + 1) != NULL)
            {
                if (access(get_token(j + 1), F_OK) == 0)
                {
                    *psaved_stdin = open(get_token(j + 1), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
                    j++; // skip next token, we already know it should be the filename
                }
                else
                {
                    printf("yash: %s: No such file or directory\n", get_token(j + 1));
                    user_input_valid = 0;
                    break;
                }
            }
            else
            {
                printf("yash: Missing filename for \"<\"\n");
                user_input_valid = 0;
                break;
            }
        }
        else if (!strcmp(get_token(j), ">"))
        {
            // this check is separate and before the other to ensure no out of bounds access.
            if (j + 1 == num_tok)
            {
                printf("yash: Missing filename for \">\"\n");
                user_input_valid = 0;
                break;
            }
            if(j == 0)
            {
               user_input_valid = 0;
               break;
            }
            if (get_token(j + 1) != NULL)
            {
                *psaved_stdout =
                    open(get_token(j + 1), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
                j++; // skip next token, we already know it should be the filename
            }
            else
            {
                printf("yash: Missing filename for \">\"\n");
                user_input_valid = 0;
                break;
            }
        }
        else if (!strcmp(get_token(j), "2>"))
        {
            // this check is separate and before the other to ensure no out of bounds access.
            if (j + 1 == num_tok)
            {
                printf("yash: Missing filename for \"2>\"\n");
                user_input_valid = 0;
                break;
            }
            if(j == 0)
            {
               user_input_valid = 0; 
               break;
            }
            if (get_token(j + 1) != NULL)
            {
                *psaved_stderr =
                    open(get_token(j + 1), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
                j++; // skip next token, we already know it should be the filename
            }
            else
            {
                printf("yash: Missing filename for \"2>\"\n");
                user_input_valid = 0;
                break;
            }
        }
        else if (!strcmp(get_token(j), "&"))
        {
            if (strcmp(get_token(j + 1), ""))
            {
                printf("yash: & must be at end of command\n");
                user_input_valid = 0;
            }
            break;
        }
        else
        {
            prog_argv[*pprog_argc] = get_token(j);
            (*pprog_argc)++;
        }
    }
    return user_input_valid;
}