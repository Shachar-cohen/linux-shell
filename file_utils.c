#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_utils.h"

#define MAX_LINE_LENGTH 1024

char** read_lines_from_file(const char *filename, int *out_line_count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("ERR");
        exit(-1);
    }
    int capacity = 10;
    int count = 0;
    char **lines = malloc(capacity * sizeof(char*));
    if (lines == NULL) {
        fprintf(stderr,"malloc");
        fclose(file);
        exit(-1);
    }
    char buffer[MAX_LINE_LENGTH];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (count == capacity) {
            capacity *= 2;
            char **temp = realloc(lines, capacity * sizeof(char*));
            if (temp == NULL) {
                fprintf(stderr,"malloc");
                break;
            }
            lines = temp;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        lines[count] = strdup(buffer);
        if (lines[count] == NULL) {
            fprintf(stderr,"ERR");
            for (int j = 0; j < count; j++) free(lines[j]);
            free(lines);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        count++;
    }
    fclose(file);
    *out_line_count = count;
    return lines;
}

bool is_dangerous_command(const char *command, char **array_of_dangerous_commands, int line_count, char **arr_of_user_command, int *dangerous_cmd_blocked, int *try_of_dangerous_cmd) {
    for (int i = 0; i < line_count; i++) {
        char dangerous_line[1024];
        strncpy(dangerous_line, array_of_dangerous_commands[i], sizeof(dangerous_line));
        dangerous_line[sizeof(dangerous_line) - 1] = '\0';

        char *dangerous_first_word = strtok(dangerous_line, " \n");

        if (dangerous_first_word != NULL && strcmp(arr_of_user_command[0], dangerous_first_word) == 0) {
            if (strcmp(array_of_dangerous_commands[i], command) == 0) {
                printf("ERR: Dangerous command detected %s execution prevented.\n", arr_of_user_command[0]);
                (*dangerous_cmd_blocked)++;
                return true;
            } else {
                (*try_of_dangerous_cmd)++;
                printf("WARNING: Command similar to dangerous command ( %s ). Proceed with caution.\n", arr_of_user_command[0]);
                return false;
            }
        }
    }
    return false;
}
