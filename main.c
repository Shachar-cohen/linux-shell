#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "command_utils.h"
#include "process_utils.h"
#include "rlimit_utils.h"
#include "signal_utils.h"
#include "file_utils.h"

#define MAX_LINE_LENGTH 1024

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "ERR: missing args. Usage: %s <dangerous_commands_file> <log_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, handler);
    signal(SIGXCPU, sigxcp_handler);
    signal(SIGSEGV, sigsegv_handler);
    signal(SIGXFSZ, sigxfsz_handler);
    signal(SIGUSR2, signofile_handler);

    int line_count = 0;
    char **array_of_dangerous_commands = read_lines_from_file(argv[1], &line_count);

    FILE *output_file = fopen(argv[2], "a");
    if (!output_file) {
        perror("ERR: cannot open log file");
        free_args(array_of_dangerous_commands);
        exit(EXIT_FAILURE);
    }

    int cmd = 0;
    int dangerous_cmd_blocked = 0;
    int try_of_dangerous_cmd = 0;
    double last_cmd_time = 0.0, avg_time = 0.0, min_time = 0.0, max_time = 0.0, sum_time = 0.0;

    while (1) {
        printf("#cmd:%d|#dangerous_cmd_blocked:%d|last_cmd_time:%f|avg_time:%f|min_time:%f|max_time:%f>>",
               cmd, dangerous_cmd_blocked, last_cmd_time, avg_time, min_time, max_time);
        fflush(stdout);

        char command[MAX_LINE_LENGTH];
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = '\0';

        int len = strlen(command);
        if (len > 0 && command[len - 1] == ' ') command[len - 1] = '\0';

        if (strcmp(command, "done") == 0) {
            printf("%d\n", (dangerous_cmd_blocked + try_of_dangerous_cmd));
            free_args(array_of_dangerous_commands);
            fclose(output_file);
            exit(0);
        }

        if (strcmp(command, "rlimit show") == 0) {
            show_all_resource_limits();
            cmd++;
            continue;
        }

        if (len == 0 || strspn(command, " ") == len) continue;
        if (has_extra_spaces(command)) continue;

        if (has_pipe(command)) {
            char before_pipe[MAX_LINE_LENGTH], after_pipe[MAX_LINE_LENGTH];
            char limits_left[MAX_LINE_LENGTH] = {0}, cmd_left[MAX_LINE_LENGTH] = {0};
            char limits_right[MAX_LINE_LENGTH] = {0}, cmd_right[MAX_LINE_LENGTH] = {0};

            split_command_to_arrays(command, before_pipe, after_pipe);

            bool has_rlimit_left = matches_rlimit_set_pattern(before_pipe);
            bool has_rlimit_right = matches_rlimit_set_pattern(after_pipe);

            if (has_rlimit_left) {
                split_rlimit_parts(before_pipe, limits_left, sizeof(limits_left), cmd_left, sizeof(cmd_left));
                if (has_too_many_args(cmd_left)) continue;
            } else {
                strcpy(cmd_left, before_pipe);
                if (has_too_many_args(cmd_left)) continue;
            }

            if (has_rlimit_right) {
                split_rlimit_parts(after_pipe, limits_right, sizeof(limits_right), cmd_right, sizeof(cmd_right));
                if (has_too_many_args(cmd_right)) continue;
            } else {
                strcpy(cmd_right, after_pipe);
                if (has_too_many_args(cmd_right)) continue;
            }

            char **new_before = split_into_7_bytes(cmd_left);
            char **new_after = split_into_7_bytes(cmd_right);

            bool flag_dangerous_command_before = is_dangerous_command(before_pipe, array_of_dangerous_commands, line_count, new_before, &dangerous_cmd_blocked, &try_of_dangerous_cmd);
            bool flag_dangerous_command_after = is_dangerous_command(after_pipe, array_of_dangerous_commands, line_count, new_after, &dangerous_cmd_blocked, &try_of_dangerous_cmd);

            if (flag_dangerous_command_before || flag_dangerous_command_after) {
                free_args(new_before);
                free_args(new_after);
                continue;
            }

            struct timeval start, end;
            gettimeofday(&start, NULL);

            int fd[2];
            if (pipe(fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t pid1 = fork();
            if (pid1 == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid1 == 0) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);

                if (has_rlimit_left) {
                    apply_rlimit_from_string(limits_left);
                }
                run_command(new_before);
            }

            pid_t pid2 = fork();
            if (pid2 == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid2 == 0) {
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);

                if (has_rlimit_right) {
                    apply_rlimit_from_string(limits_right);
                }
                run_command(new_after);
            }

            close(fd[0]);
            close(fd[1]);

            int status1, status2;
            waitpid(pid1, &status1, 0);
            waitpid(pid2, &status2, 0);

            print_exit_status(status1);
            print_exit_status(status2);

            if (WIFEXITED(status1) && WEXITSTATUS(status1) == 0) {
                gettimeofday(&end, NULL);
                update_command_time(before_pipe, start, end, output_file, &last_cmd_time, &sum_time, &min_time, &max_time, &avg_time, &cmd);
            }
            if (WIFEXITED(status2) && WEXITSTATUS(status2) == 0) {
                gettimeofday(&end, NULL);
                update_command_time(after_pipe, start, end, output_file, &last_cmd_time, &sum_time, &min_time, &max_time, &avg_time, &cmd);
            }

            free_args(new_before);
            free_args(new_after);

        } else {
            bool there_is_ampersand = has_ampersand(command);
            if (there_is_ampersand) {
                remove_ampersand(command);
            }
            bool match_rlimit = matches_rlimit_set_pattern(command);
            char **arr_of_user_command = NULL;
            char **arr_of_user_command_part = NULL;
            char limits_part[MAX_LINE_LENGTH];

            if (match_rlimit) {
                char command_part[MAX_LINE_LENGTH];
                split_rlimit_parts(command, limits_part, sizeof(limits_part), command_part, sizeof(command_part));
                if (has_too_many_args(command_part)) {
                    continue;
                }
                arr_of_user_command_part = split_into_7_bytes(command_part);
                if (is_dangerous_command(command_part, array_of_dangerous_commands, line_count, arr_of_user_command_part, &dangerous_cmd_blocked, &try_of_dangerous_cmd)) {
                    continue;
                }
            } else {
                if (has_too_many_args(command)) continue;
                arr_of_user_command = split_into_7_bytes(command);
                if (is_dangerous_command(command, array_of_dangerous_commands, line_count, arr_of_user_command, &dangerous_cmd_blocked, &try_of_dangerous_cmd)) {
                    continue;
                }
            }

            struct timeval start, end;
            gettimeofday(&start, NULL);

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            }

            if (pid == 0) {
                if (has_stderr_redirect(command)) {
                    redirect_stderr_to_file_from_cmd(command);
                }
                if (!match_rlimit) {
                    run_command(arr_of_user_command);
                } else {
                    apply_rlimit_from_string(limits_part);
                    run_command(arr_of_user_command_part);
                }
            } else {
                if (!there_is_ampersand) {
                    int status;
                    wait(&status);
                    print_exit_status(status);
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        gettimeofday(&end, NULL);
                        update_command_time(command, start, end, output_file, &last_cmd_time, &sum_time, &min_time, &max_time, &avg_time, &cmd);
                    }
                }
                if (match_rlimit) {
                    free_args(arr_of_user_command_part);
                } else {
                    free_args(arr_of_user_command);
                }
            }
        }
    }

    free_args(array_of_dangerous_commands);
    fclose(output_file);
    return 0;
}
