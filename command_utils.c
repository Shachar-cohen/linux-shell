#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command_utils.h"

bool has_extra_spaces(const char *str) {
    int len = strlen(str);
    if (len > 1024) {
        fprintf(stderr,"ERR\n");
        return true;
    }
    for (int i = 0; i < len - 1; i++) {
        if (str[i] == ' ' && str[i + 1] == ' ') {
           fprintf(stderr,"ERR_SPACE\n");
            return true;
        }
    }
    return false;
}

bool has_too_many_args(const char *str) {
    int len = strlen(str);
    int word_count = 0;
    bool in_word = false;
    for (int i = 0; i < len; i++) {
        if (str[i] == ' ') {
            in_word = false;
        } else {
            if (!in_word) {
                word_count++;
                in_word = true;
            }
        }
    }
    if (word_count > 7) {
        fprintf(stderr,"ERR_ARGS\n");
        return true;
    }
    return false;
}

char** split_into_7_bytes(const char* command) {
    char* command_copy = strdup(command);
    if (command_copy == NULL) {
        fprintf(stderr ,"strdup failed");
        exit(EXIT_FAILURE);
    }
    char** result = malloc(8 * sizeof(char*));
    if (result == NULL) {
        fprintf(stderr, "malloc failed");
        free(command_copy);
        exit(EXIT_FAILURE);
    }
    int i = 0;
    char* token = strtok(command_copy, " ");
    while (token != NULL && i < 7) {
        result[i] = strdup(token);
        if (result[i] == NULL) {
            fprintf(stderr,"strdup failed");
            for (int j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            free(command_copy);
            exit(EXIT_FAILURE);
        }
        i++;
        token = strtok(NULL, " ");
    }
    result[i] = NULL;
    free(command_copy);
    return result;
}

bool has_pipe(const char *command) {
    const char *p = command;
    while ((p = strchr(p, '|')) != NULL) {
        if (p != command && *(p - 1) == ' ' && *(p + 1) == ' ') {
            return true;
        }
        p++;
    }
    return false;
}

void split_command_to_arrays(const char *command, char *before_pipe, char *after_pipe) {
    const char *pipe_pos = strchr(command, '|');
    if (!pipe_pos) {
        before_pipe[0] = '\0';
        after_pipe[0] = '\0';
        return;
    }
    const char *end_before = pipe_pos;
    while (end_before > command && *(end_before - 1) == ' ') {
        end_before--;
    }
    int len_before = end_before - command;
    strncpy(before_pipe, command, len_before);
    before_pipe[len_before] = '\0';

    const char *start_after = pipe_pos + 1;
    while (*start_after == ' ') {
        start_after++;
    }
    strcpy(after_pipe, start_after);
}

bool has_ampersand(const char *cmd) {
    int len = strlen(cmd);
    while (len > 0 && cmd[len - 1] == ' ') len--;
    return len > 0 && cmd[len - 1] == '&';
}

void remove_ampersand(char *cmd) {
    int len = strlen(cmd);
    while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '&')) {
        cmd[--len] = '\0';
    }
}

void free_args(char **args) {
    if (!args) return;
    for (int i = 0; i < 7 && args[i]; i++) {
        free(args[i]);
    }
    free(args);
}

bool matches_rlimit_set_pattern(const char *line) {
    if (!line || strlen(line) == 0) return false;
    char copy[2048];
    strncpy(copy, line, sizeof(copy));
    copy[sizeof(copy) - 1] = '\0';

    char *tokens[64];
    int count = 0;
    char *token = strtok(copy, " \t\n");
    while (token && count < 64) {
        tokens[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    if (count < 4) return false;
    if (strcmp(tokens[0], "rlimit") != 0 || strcmp(tokens[1], "set") != 0)
        return false;

    int i = 2;
    bool found_limit = false;
    for (; i < count; i++) {
        if (strchr(tokens[i], '=')) {
            found_limit = true;
        } else {
            break;
        }
    }
    if (!found_limit) return false;
    if (i >= count) return false;

    return true;
}

void split_rlimit_parts(const char *line, char *limits_part, size_t limits_size, char *command_part, size_t command_size) {
    if (!line || !limits_part || !command_part) return;
    char copy[2048];
    strncpy(copy, line, sizeof(copy));
    copy[sizeof(copy) - 1] = '\0';

    char *tokens[64];
    int count = 0;
    char *token = strtok(copy, " \t\n");
    while (token && count < 64) {
        tokens[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    if (count < 4 || strcmp(tokens[0], "rlimit") != 0 || strcmp(tokens[1], "set") != 0)
        return;

    limits_part[0] = '\0';
    command_part[0] = '\0';

    strcat(limits_part, "rlimit set");
    int i = 2;
    for (; i < count && strchr(tokens[i], '=') != NULL; i++) {
        strcat(limits_part, " ");
        strcat(limits_part, tokens[i]);
    }
    for (; i < count; i++) {
        strcat(command_part, tokens[i]);
        if (i < count - 1) strcat(command_part, " ");
    }
}
