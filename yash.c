#include <evaluate.h>
#include <parser.h>
#include <yash.h>

extern char** environ;

char TOKENS[MAX_INPUT_SIZE][MAX_TOKEN_SIZE] = {0};

process_stack_t process_stack = {
    .top = -1,
    .arr = {-1},
    .status = {0},
    .user_str = {{0}},
    .size = 0,
    .job_id = {-1},
};

int at_prompt = 0;
int top_process_i = -1;
int shell_terminal;
pid_t shell_pgid;
struct termios shell_tmodes;
char done_to_print[MAX_INPUT_SIZE * MAX_INPUT_SIZE]; // really shouldnt need to be bigger than this...

int max(int arr[], int n)
{
    int max = arr[0];
    for (int i = 1; i < n; i++)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }
    }
    return max;
}

int assign_job_id()
{
    return 1 + max(process_stack.job_id, PROCESS_STACK_DEPTH);
}

pid_t get_shell_pgid()
{
    return shell_pgid;
}

int get_shell_terminal()
{
    return shell_terminal;
}

char *get_token(int i)
{
    return TOKENS[i];
}

void set_token(char *token, int i)
{
    strncpy(TOKENS[i], token, MAX_TOKEN_SIZE);
}

static int get_top_process()
{
    int process_stack_i = process_stack.size - 1;
    if (process_stack_i == -1)
        return 1;
    while (process_stack.arr[process_stack_i] <= 0)
    {
        process_stack_i--;
    }
    return process_stack_i;
}

void child_handler()
{
    int status;
    int errno;
    pid_t pid;
    do
    {
        errno = 0;
        pid = waitpid(WAIT_ANY, &status, WNOHANG | WUNTRACED|WCONTINUED);
    } while (pid <= 0 && errno == EINTR);

    int i = 0;
    while ((process_stack.arr[i] != pid) && (i < PROCESS_STACK_DEPTH))
    {
        i++;
    }
    if (WIFEXITED(status) && (i != PROCESS_STACK_DEPTH) && (pid > 0) && !(WEXITSTATUS(status) == EXIT_FAILURE))
    {
        process_stack.status[i] = DONE;
        if (get_top_process() == i)
        {
            sprintf(done_to_print + strlen(done_to_print), "[%d]%c %s\t%s\n", process_stack.job_id[i], '+', "Done",
                    (process_stack.user_str[i]));
        }
        else
        {
            sprintf(done_to_print + strlen(done_to_print), "[%d]%c %s\t%s\n", process_stack.job_id[i], '-', "Done",
                    (process_stack.user_str[i]));
        }

        process_stack.arr[i] = -1;
        process_stack.job_id[i] = -1;
        process_stack.status[i] = NONE;
    }
    if (WEXITSTATUS(status) == EXIT_FAILURE)
    {
        // if process fails to execute, remove it from jobs immediately
        process_stack.arr[i] = -1;
        process_stack.job_id[i] = -1;
        process_stack.status[i] = NONE;
    }
}

static void spawn_process(char *argv[], pid_t pgid, int infile, int outfile, int errfile, int fg)
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
    execvpe(argv[0], argv, environ);
    perror("yash: Failed to execute process\n");
    exit(1);
}

int main(int argc, char **argv)
{
    char *user_str;

    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
        kill(-shell_pgid, SIGTTIN);

    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGSTOP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, child_handler);

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

        memset(TOKENS, 0, MAX_INPUT_SIZE * MAX_TOKEN_SIZE * sizeof(TOKENS[0][0]));
        memset(jprog_argv, 0, MAX_INPUT_SIZE * sizeof(jprog_argv[0]));
        memset(kprog_argv, 0, MAX_INPUT_SIZE * sizeof(kprog_argv[0]));

        int status = 0;
        int user_input_valid = 1;
        int bg = 0;
        int num_tok = 0;
        int num_tok_before_pipe = 0;

        fflush(stdout);
        at_prompt = 1;
        user_str = readline("# ");
        printf("%s", done_to_print);
        done_to_print[0] = 0;
        if (user_str == NULL)
        {
            // Ctrl-D (EOF) encountered
            exit(0);
        }
        at_prompt = 0;

        char user_str_deep_copy[MAX_INPUT_SIZE];
        strncpy(user_str_deep_copy, user_str, MAX_INPUT_SIZE);

        if (parse_user_input(user_str, &num_tok_before_pipe, &bg))
        {
            user_input_valid = 0;
        }

        while (strcmp(TOKENS[num_tok], ""))
        {
            num_tok++;
        }

        user_input_valid = evaluate_command_tokens(num_tok_before_pipe, num_tok, num_tok, &jsaved_stdin, &jsaved_stdout,
                                                   &jsaved_stderr, jprog_argv, &jprog_argc, &process_stack);

        if ((0 < num_tok_before_pipe) && (num_tok_before_pipe < num_tok))
        {
            // &= because both process need valid input
            user_input_valid &=
                evaluate_command_tokens(0, num_tok_before_pipe - 1, num_tok_before_pipe - 1, &ksaved_stdin,
                                        &ksaved_stdout, &ksaved_stderr, kprog_argv, &kprog_argc, &process_stack);

            if (user_input_valid)
            {
                pipe(my_pipe);
                pid_t lpid = fork();
                if (lpid == (pid_t)0)
                {
                    // k child
                    close(my_pipe[0]);
                    dup2(my_pipe[1], STDOUT_FILENO);
                    if(jsaved_stdin == STDOUT_FILENO)
                    {
                        spawn_process(kprog_argv, lpid, ksaved_stdin, my_pipe[1], ksaved_stderr, 1);
                    }
                    else
                    {
                        spawn_process(kprog_argv, lpid, ksaved_stdin, ksaved_stdout, ksaved_stderr, 1);
                    }
                }
                // parent, spawning j child
                pid_t rpid = fork();
                if (rpid == (pid_t)0)
                {
                    close(my_pipe[1]);
                    dup2(my_pipe[0], STDIN_FILENO);
                    if(jsaved_stdin == STDIN_FILENO)
                    {
                        spawn_process(jprog_argv, lpid, my_pipe[0], jsaved_stdout, jsaved_stderr, 1);
                    }
                    else
                    {
                        spawn_process(jprog_argv, lpid, jsaved_stdin, jsaved_stdout, jsaved_stderr, 1);
                    }
                }
                // parent
                close(my_pipe[0]);
                close(my_pipe[1]);
                tcsetpgrp(shell_terminal, lpid);
                waitpid(-lpid, &status, WUNTRACED|WCONTINUED);
                tcsetpgrp(shell_terminal, shell_pgid);
                if (WIFSTOPPED(status) && !WIFEXITED(status))
                {
                    process_stack.top++;
                    process_stack.size++;
                    process_stack.arr[process_stack.top] = lpid;
                    process_stack.job_id[process_stack.top] = assign_job_id();
                    process_stack.status[process_stack.top] = STOPPED;
                    strcpy(process_stack.user_str[process_stack.top], user_str_deep_copy);
                }
                tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
            }
        }
        else
        {
            if (user_input_valid)
            {
                pid_t pid = fork();
                if (pid == (pid_t)0)
                {
                    spawn_process(jprog_argv, pid, jsaved_stdin, jsaved_stdout, jsaved_stderr, !bg);
                }
                else
                {
                    // this is the parent (shell)
                    if (bg)
                    {
                        if (kill(pid, SIGCONT) < 0)
                            perror("kill (SIGCONT)");
                        tcsetpgrp(shell_terminal, shell_pgid);
                        process_stack.top++;
                        process_stack.size++;
                        process_stack.arr[process_stack.top] = pid;
                        process_stack.job_id[process_stack.top] = assign_job_id();
                        process_stack.status[process_stack.top] = RUNNING;
                        strcpy(process_stack.user_str[process_stack.top], user_str_deep_copy);
                    }
                    else
                    {
                        tcsetpgrp(shell_terminal, pid);
                        waitpid(pid, &status, WUNTRACED|WCONTINUED);
                        tcsetpgrp(shell_terminal, shell_pgid);
                        if (WIFSTOPPED(status) && !WIFEXITED(status))
                        {
                            process_stack.top++;
                            process_stack.size++;
                            process_stack.arr[process_stack.top] = pid;
                            process_stack.job_id[process_stack.top] = assign_job_id();
                            process_stack.status[process_stack.top] = STOPPED;
                            strcpy(process_stack.user_str[process_stack.top], user_str_deep_copy);
                        }
                    }
                    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                }
            }
            else
            {
                printf("yash: Invalid input\n");
            }
        }
        free(user_str);
    }
    return 0;
}
