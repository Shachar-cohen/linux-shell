#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include "process_utils.h"
#include  <signal.h>

void run_command(char **command) {
    if (strcmp(command[0], "my_tee") == 0) {
        my_tee(command);
        exit(EXIT_SUCCESS);
    }
    if (execvp(command[0], command) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}

void my_tee(char **args) {
    bool append = false;
    int i = 1;
    if (args[i] && strcmp(args[i], "-a") == 0) {
        append = true;
        i++;
    }
    int file_count = 0;
    while (args[i + file_count]) {
        file_count++;
    }
    if (file_count == 0) {
        fprintf(stderr, "my_tee: missing file operand\n");
        exit(EXIT_FAILURE);
    }
    int fds[file_count];
    for (int j = 0; j < file_count; j++) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        fds[j] = open(args[i + j], flags, 0644);
        if (fds[j] < 0) {
            perror("my_tee: open");
            for (int k = 0; k < j; k++) close(fds[k]);
            exit(EXIT_FAILURE);
        }
    }
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, bytes_read) < 0) {
            perror("my_tee: write to stdout");
            break;
        }
        for (int j = 0; j < file_count; j++) {
            if (write(fds[j], buffer, bytes_read) < 0) {
                perror("my_tee: write to file");
                break;
            }
        }
    }
    if (bytes_read < 0) {
        perror("my_tee: read from stdin");
    }
    for (int j = 0; j < file_count; j++) {
        close(fds[j]);
    }
}

bool has_stderr_redirect(const char *cmd) {
    return strstr(cmd, "2>") != NULL;
}

void redirect_stderr_to_file_from_cmd(const char *cmd) {
    const char *marker = strstr(cmd, "2>");
    if (!marker) return;
    const char *filename = marker + 3;
    if (*filename == '\0') return;
    const char *end = filename;
    while (*end && *end != ' ') end++;
    size_t len = end - filename;
    char *clean_filename = malloc(len + 1);
    if (!clean_filename) return;
    strncpy(clean_filename, filename, len);
    clean_filename[len] = '\0';

    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    int fd = open(clean_filename, flags, 0600);
    free(clean_filename);
    if (fd < 0) {
        perror("open");
        return;
    }
    dup2(fd, STDERR_FILENO);
    close(fd);
}

void print_exit_status(int status) {
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        switch (sig) {
            case SIGXCPU:
                fprintf(stderr, "CPU time limit exceeded!\n");
                break;
            case SIGXFSZ:
                fprintf(stderr, "File size limit exceeded!\n");
                break;
            case SIGSEGV:
                fprintf(stderr, "Memory allocation failed!\n");
                break;
            default:
                fprintf(stderr, "[EXIT] Terminated by signal: %s\n", strsignal(sig));
        }
    } else if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code != 0) {
            fprintf(stderr, "[EXIT] Process exited with error code %d\n", code);
        }
    } else {
        fprintf(stderr, "[EXIT] Process ended abnormally\n");
    }
}

void update_command_time(const char *command, struct timeval start, struct timeval end,
                         FILE *output_file, double *last_cmd_time, double *sum_time,
                         double *min_time, double *max_time, double *avg_time, int *cmd_count) {
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    if (last_cmd_time) *last_cmd_time = time_spent;
    if (cmd_count) (*cmd_count)++;
    fprintf(output_file, "%s : %.5f sec\n", command, time_spent);
    fflush(output_file);
    if (sum_time) *sum_time += time_spent;
    if (min_time && max_time && cmd_count) {
        if (*cmd_count == 1) {
            *min_time = time_spent;
            *max_time = time_spent;
        } else {
            if (time_spent < *min_time) *min_time = time_spent;
            if (time_spent > *max_time) *max_time = time_spent;
        }
    }
    if (avg_time && sum_time && cmd_count) {
        *avg_time = *sum_time / *cmd_count;
    }
}
