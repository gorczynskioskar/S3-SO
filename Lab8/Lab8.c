#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
static bool is_natural_number_str(const char* s) {
    if (!s || *s == '\0') return 0;
    for (const char* p = s; *p; ++p) {
        if (!isdigit((unsigned char)*p)) return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 2){
        fprintf(stderr, "Error: only one argument needed.\n");
		return 1;
    }
	if (!is_natural_number_str(argv[1])) {
		fprintf(stderr, "Error: argument must be a natural number.\n");
		return 2;
	}

    int n = atoi(argv[1]);
    if (n < 1 || n > 13) {
        fprintf(stderr, "Error: argument must be a number between 1 and 13.\n");
        return 3;
    }

    if (n < 1 || n > 13) {
        fprintf(stderr, "Error: argument must be a number between 1 and 13.\n");
        return 3;
    }

    if (n == 1 || n == 2) {
        return 1;
    }

    long arg1 = n - 1;
    long arg2 = n - 2;

    pid_t child1 = fork();
    if (child1 < 0) {
        perror("fork (child1)");
        return 1;
    }
    if (child1 == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", arg1);
        char* new_argv[] = { argv[0], buf, NULL };
        execv(argv[0], new_argv);
        perror("execv (child1)");
        _exit(127);
    }

    pid_t child2 = fork();
    if (child2 < 0) {
        perror("fork (child2)");
        int tmp_status;
        (void)waitpid(child1, &tmp_status, 0);
        return 1;
    }
    if (child2 == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", arg2);
        char* new_argv[] = { argv[0], buf, NULL };
        execv(argv[0], new_argv);
        perror("execv (child2)");
        _exit(127);
    }

    struct child_info {
        pid_t pid;
        long  arg;
        int   exit_code;
    } finished[2];

    for (int i = 0; i < 2; ++i) {
        int status = 0;
        pid_t pid = wait(&status);
        if (pid < 0) {
            perror("wait");
            return 1;
        }
        int code = 0;
        if (WIFEXITED(status)) {
            code = WEXITSTATUS(status);
        }
        else {
            code = 0;
        }

        long child_arg = (pid == child1) ? arg1 :
            (pid == child2) ? arg2 : -1;

        finished[i].pid = pid;
        finished[i].arg = child_arg;
        finished[i].exit_code = code;
    }

    pid_t parent_pid = getpid();
    printf("%d %d %ld %d\n",
        (int)parent_pid, (int)finished[0].pid, finished[0].arg, finished[0].exit_code);
    printf("%d %d %ld %d\n",
        (int)parent_pid, (int)finished[1].pid, finished[1].arg, finished[1].exit_code);
    int sum_codes = finished[0].exit_code + finished[1].exit_code;
    printf("%d %d\n", (int)parent_pid, sum_codes);

    return sum_codes;

}
