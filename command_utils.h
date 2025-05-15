#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include <stdbool.h>
#include <stddef.h>

bool has_too_many_args(const char *str);
bool has_extra_spaces(const char *str);
char** split_into_7_bytes(const char *command);
bool has_pipe(const char *command);
void split_command_to_arrays(const char *command, char *before_pipe, char *after_pipe);
bool has_ampersand(const char *cmd);
void remove_ampersand(char *cmd);
void free_args(char **args);
bool matches_rlimit_set_pattern(const char *line);
void split_rlimit_parts(const char *line, char *limits_part, size_t limits_size, char *command_part, size_t command_size);

#endif



