#ifndef PROCESS_UTILS_H
#define PROCESS_UTILS_H

#include <stdio.h>
#include <sys/time.h>

void run_command(char **command);
void my_tee(char **args);
bool has_stderr_redirect(const char *cmd);
void redirect_stderr_to_file_from_cmd(const char *cmd);
void print_exit_status(int pid);
void update_command_time(const char *command, struct timeval start, struct timeval end,
                         FILE *output_file, double *last_cmd_time, double *sum_time,
                         double *min_time, double *max_time, double *avg_time, int *cmd_count);

#endif