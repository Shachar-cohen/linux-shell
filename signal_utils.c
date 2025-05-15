#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "signal_utils.h"

void handler(int sig_num) {
    (void)sig_num;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // reap all children
    }
}

void sigxcp_handler(int signum) {
    (void)signum;
    const char *msg = "CPU time limit exceeded!\n";
    write(STDERR_FILENO, msg, strlen(msg));
    _exit(1);
}

void sigsegv_handler(int signum) {
    (void)signum;
    const char *msg = "Memory allocation failed!\n";
    write(STDERR_FILENO, msg, strlen(msg));
    _exit(1);
}

void sigxfsz_handler(int signum) {
    (void)signum;
    const char *msg = "File size limit exceeded!\n";
    write(STDERR_FILENO, msg, strlen(msg));
    _exit(1);
}

void signofile_handler(int signum) {
    (void)signum;
    const char *msg = "Too many open files!\n";
    write(STDERR_FILENO, msg, strlen(msg));
    _exit(1);
}

