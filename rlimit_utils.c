#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include "rlimit_utils.h"

void apply_rlimit_from_string(const char *line) {
    if (!line) return;

    char buffer[2048];
    strncpy(buffer, line, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char *tokens[64];
    int count = 0;

    char *token = strtok(buffer, " \t\n");
    while (token && count < 64) {
        tokens[count++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (count < 4 || strcmp(tokens[0], "rlimit") != 0 || strcmp(tokens[1], "set") != 0)
        return;

    char *limits[16] = {NULL};
    int i = 2, limit_count = 0;

    while (i < count && strchr(tokens[i], '=')) {
        limits[limit_count++] = tokens[i++];
    }

    limits[limit_count] = NULL;

    for (int j = 0; j < limit_count; j++) {
        char *eq = strchr(limits[j], '=');
        if (!eq) continue;

        char resource[32] = {0};
        strncpy(resource, limits[j], eq - limits[j]);

        char *val = eq + 1;
        char *colon = strchr(val, ':');

        char soft_str[32] = {0}, hard_str[32] = {0};
        if (colon) {
            strncpy(soft_str, val, colon - val);
            soft_str[colon - val] = '\0';
            strcpy(hard_str, colon + 1);
        } else {
            strcpy(soft_str, val);
            strcpy(hard_str, val);
        }

        rlim_t soft = 0, hard = 0;
        if (strcmp(resource, "cpu") == 0 || strcmp(resource, "nofile") == 0) {
            soft = atoi(soft_str);
            hard = atoi(hard_str);
        } else {
            char *end1, *end2;
            soft = strtoul(soft_str, &end1, 10);
            hard = strtoul(hard_str, &end2, 10);

            // כאן אפשר להוסיף המרה של K, M, G אם תרצה
        }

        int res_type = -1;
        if (strcmp(resource, "cpu") == 0) res_type = RLIMIT_CPU;
        else if (strcmp(resource, "fsize") == 0) res_type = RLIMIT_FSIZE;
        else if (strcmp(resource, "mem") == 0 || strcmp(resource, "data") == 0) res_type = RLIMIT_DATA;
        else if (strcmp(resource, "nofile") == 0) res_type = RLIMIT_NOFILE;

        if (res_type != -1) {
            struct rlimit lim = {soft, hard};
            if (setrlimit(res_type, &lim) != 0) {
                perror("setrlimit failed");
            }
        }
    }
}

void show_all_resource_limits() {
    struct rlimit lim;

    if (getrlimit(RLIMIT_CPU, &lim) == 0) {
        printf("CPU time: soft=%lu, hard=%lu\n",
               (unsigned long)lim.rlim_cur, (unsigned long)lim.rlim_max);
    }

    if (getrlimit(RLIMIT_DATA, &lim) == 0) {
        printf("Memory: soft=%lu, hard=%lu\n",
               (unsigned long)lim.rlim_cur, (unsigned long)lim.rlim_max);
    }

    if (getrlimit(RLIMIT_FSIZE, &lim) == 0) {
        printf("File size: soft=%lu, hard=%lu\n",
               (unsigned long)lim.rlim_cur, (unsigned long)lim.rlim_max);
    }

    if (getrlimit(RLIMIT_NOFILE, &lim) == 0) {
        printf("Open files: soft=%lu, hard=%lu\n",
               (unsigned long)lim.rlim_cur, (unsigned long)lim.rlim_max);
    }
}

