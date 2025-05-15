#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdbool.h>

char** read_lines_from_file(const char *filename, int *out_line_count);
bool is_dangerous_command(const char *command, char **array_of_dangerous_commands, int line_count, char **arr_of_user_command, int *dangerous_cmd_blocked, int *try_of_dangerous_cmd);

#endif
