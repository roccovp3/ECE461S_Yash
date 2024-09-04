#include <shell_commands.h>

extern char TOKENS[MAX_INPUT_SIZE][MAX_TOKEN_SIZE];

int shell_builtin_commands(process_stack_t *pprocess_stack, int num_tok)
{
    int status;
    if (!strcmp(TOKENS[0], "fg") && (num_tok == 1))
    {
        if ((pprocess_stack->top) < 0)
        {
            return 1;
        }
        while ((pprocess_stack->arr[pprocess_stack->top] < 0))
        {
            pprocess_stack->top--;
            if ((pprocess_stack->top) == -1)
            {
                return 1;
            }
        }

        if (kill((pprocess_stack->arr)[pprocess_stack->top], SIGCONT) == 0)
        {
            struct termios shell_tmodes;
            int status;

            tcgetattr(get_shell_terminal(), &shell_tmodes);
            tcsetpgrp(get_shell_terminal(), pprocess_stack->arr[pprocess_stack->top]);
            printf("%s\n", (pprocess_stack->user_str[pprocess_stack->top]));
            pprocess_stack->status[pprocess_stack->top] = RUNNING;
            waitpid(pprocess_stack->arr[pprocess_stack->top], &status, WUNTRACED);
            tcsetpgrp(get_shell_terminal(), get_shell_pgid());
            if (WIFSTOPPED(status) && !WIFEXITED(status))
            {
                pprocess_stack->status[pprocess_stack->top] = STOPPED;
            }
            else
            {
                pprocess_stack->arr[pprocess_stack->top] = -1;
                pprocess_stack->status[pprocess_stack->top] = NONE;
                pprocess_stack->top--;
            }
            tcsetattr(get_shell_terminal(), TCSADRAIN, &shell_tmodes);

            return 1;
        }
        else
        {
            if (pprocess_stack->status[pprocess_stack->top] == DONE)
            {
                pprocess_stack->arr[pprocess_stack->top] = -1;
                pprocess_stack->status[pprocess_stack->top] = NONE;
                pprocess_stack->top--;
            }
        }
        return 1;
    }
    else if (!strcmp(TOKENS[0], "bg") && (num_tok == 1))
    {
        if(pprocess_stack->top < 0)
        {
            printf("No stopped process in the background\n");
            return 1;
        }
        while ((pprocess_stack->status[pprocess_stack->top] != STOPPED))
        {
            pprocess_stack->top--;
            if ((pprocess_stack->top) == -1)
            {
                printf("No stopped process in the background\n");
                return 1;
            }
        }
        while (1)
        {
            // process no longer stopped
            if (((pprocess_stack->top) == -1))
            {
                printf("No stopped process in the background\n");
                return 1;
            }

            if ((kill((pprocess_stack->arr)[pprocess_stack->top], SIGCONT)) == 0)
            {
                struct termios shell_tmodes;
                tcgetattr(get_shell_terminal(), &shell_tmodes);
                pid_t pid = pprocess_stack->arr[pprocess_stack->top];
                pprocess_stack->status[pprocess_stack->top] = RUNNING;
                tcsetpgrp(get_shell_terminal(), pid);
                printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack->top], '+', "Running", (pprocess_stack->user_str[pprocess_stack->top]));
                waitpid(pid, &status, WNOHANG | WUNTRACED);
                tcsetpgrp(get_shell_terminal(), get_shell_pgid());
                tcsetattr(get_shell_terminal(), TCSADRAIN, &shell_tmodes);
                return 1;
            }
        }

        printf("No processes in the background\n");
        return 1;
    }
    else if (!strcmp(TOKENS[0], "jobs") && (num_tok == 1))
    {
        int pprocess_stack_i = pprocess_stack->size - 1;
        if (pprocess_stack_i == -1)
            return 1;
        while (pprocess_stack->arr[pprocess_stack_i] <= 0)
        {
            pprocess_stack_i--;
        }
        if (pprocess_stack->status[pprocess_stack_i] == STOPPED)
        {
            printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack_i], '+', "Stopped",
                   (pprocess_stack->user_str[pprocess_stack_i]));
        }
        else if (pprocess_stack->status[pprocess_stack_i] == DONE)
        {
            printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack_i], '+', "Done",
                   (pprocess_stack->user_str[pprocess_stack_i]));
        }
        else if (pprocess_stack->status[pprocess_stack_i] == RUNNING)
        {
            printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack_i], '+', "Running",
                   (pprocess_stack->user_str[pprocess_stack_i]));
        }
        pprocess_stack_i--;
        while (pprocess_stack_i >= 0)
        {
            if (pprocess_stack->arr[pprocess_stack_i] > 0)
            {
                if (pprocess_stack->status[pprocess_stack_i] == STOPPED)
                {
                    printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack_i], '-', "Stopped",
                           (pprocess_stack->user_str[pprocess_stack_i]));
                }
                else if (pprocess_stack->status[pprocess_stack_i] == DONE)
                {
                    printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack_i], '-', "Done",
                           (pprocess_stack->user_str[pprocess_stack_i]));
                }
                else if (pprocess_stack->status[pprocess_stack_i] == RUNNING)
                {
                    printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack_i], '-', "Running",
                           (pprocess_stack->user_str[pprocess_stack_i]));
                }
            }
            pprocess_stack_i--;
            while (pprocess_stack->arr[pprocess_stack_i] <= 0)
            {
                pprocess_stack_i--;
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
}