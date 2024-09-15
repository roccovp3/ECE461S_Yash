#include <shell_commands.h>

extern char TOKENS[MAX_INPUT_SIZE][MAX_TOKEN_SIZE];

int shell_builtin_commands(process_stack_t *pprocess_stack, int num_tok)
{
    int status;
    if (!strcmp(TOKENS[0], "fg") && (num_tok == 1))
    {
        int pprocess_stack_i = pprocess_stack->size - 1;
        if ((pprocess_stack_i) < 0)
        {
            return 1;
        }
        while ((pprocess_stack->arr[pprocess_stack_i] <= 0))
        {
            pprocess_stack_i--;
            if ((pprocess_stack_i) == -1)
            {
                return 1;
            }
        }

        kill((pprocess_stack->arr)[pprocess_stack_i], SIGCONT);
        struct termios shell_tmodes;
        int status;

        tcgetattr(get_shell_terminal(), &shell_tmodes);
        tcsetpgrp(get_shell_terminal(), pprocess_stack->arr[pprocess_stack_i]);
        printf("%s\n", (pprocess_stack->user_str[pprocess_stack_i]));
        pprocess_stack->status[pprocess_stack_i] = RUNNING;
        waitpid(-pprocess_stack->arr[pprocess_stack_i], &status, WUNTRACED);
        tcsetpgrp(get_shell_terminal(), get_shell_pgid());
        if (WIFSTOPPED(status) && !WIFEXITED(status))
        {
            pprocess_stack->status[pprocess_stack_i] = STOPPED;
        }
        if(WIFEXITED(status))
        {
            pprocess_stack->arr[pprocess_stack_i] = -1;
            pprocess_stack->job_id[pprocess_stack_i] = -1;
            pprocess_stack->status[pprocess_stack_i] = NONE;
        }
        tcsetattr(get_shell_terminal(), TCSADRAIN, &shell_tmodes);
        return 1;
    }
    else if (!strcmp(TOKENS[0], "bg") && (num_tok == 1))
    {
        int pprocess_stack_i = pprocess_stack->size - 1;
        if (pprocess_stack_i < 0)
        {
            //printf("No stopped process in the background\n");
            return 1;
        }
        while ((pprocess_stack->status[pprocess_stack_i] != STOPPED))
        {
            pprocess_stack_i--;
            if ((pprocess_stack_i) == -1)
            {
                //printf("No stopped process in the background\n");
                return 1;
            }
        }
        while (1)
        {
            // process no longer stopped
            if (((pprocess_stack_i) == -1))
            {
                //printf("No stopped process in the background\n");
                return 1;
            }

            if ((kill((pprocess_stack->arr)[pprocess_stack_i], SIGCONT)) == 0)
            {
                struct termios shell_tmodes;
                tcgetattr(get_shell_terminal(), &shell_tmodes);
                pid_t pid = pprocess_stack->arr[pprocess_stack_i];
                pprocess_stack->status[pprocess_stack_i] = RUNNING;
                tcsetpgrp(get_shell_terminal(), pid);
                printf("[%d]%c %s\t%s\n", pprocess_stack->job_id[pprocess_stack_i], '+', "Running",
                       (pprocess_stack->user_str[pprocess_stack_i]));
                waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
                tcsetpgrp(get_shell_terminal(), get_shell_pgid());
                tcsetattr(get_shell_terminal(), TCSADRAIN, &shell_tmodes);
                return 1;
            }
        }

        //printf("No processes in the background\n");
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