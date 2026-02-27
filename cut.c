#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char delim = '\t'; // Default delimiter is TAB
    int fields[100];
    int n_fields = 0;
    char line[1024];

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-d", 2) == 0 || strcmp(argv[i], "--delimiter") == 0) {
            if (strncmp(argv[i], "-d", 2) == 0 && strlen(argv[i]) > 2) {
                delim = argv[i][2]; // -d:
            } else if (i + 1 < argc) {
                delim = argv[++i][0]; // -d ":"
            }
        } 
        else if (strncmp(argv[i], "-f", 2) == 0 || strcmp(argv[i], "--fields") == 0) {
            char *field_str = NULL;

            if (strncmp(argv[i], "-f", 2) == 0 && strlen(argv[i]) > 2) {
                field_str = argv[i] + 2; // -f1,3,10
            } else if (i + 1 < argc) {
                field_str = argv[++i];    // -f 1,3,10 or --fields 1,3,10
            }

            if (field_str) {
                char *field_copy = strdup(field_str);
                char *token = strtok(field_copy, ",");
                while (token != NULL) {
                    fields[n_fields++] = atoi(token);
                    token = strtok(NULL, ",");
                }
                free(field_copy);
            }
        }
    }

    if (n_fields == 0) return 1;

    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;
        char *parts[100];
        int part_count = 0;
        char *current = line;
        
        parts[part_count++] = current;
        while (*current) {
            if (*current == delim) {
                *current = '\0';
                parts[part_count++] = current + 1;
            }
            current++;
        }

        for (int i = 0; i < n_fields; i++) {
            int target = fields[i] - 1;
            if (target >= 0 && target < part_count) {
                printf("%s", parts[target]);
            }
            if (i < n_fields - 1) putchar(delim);
        }
        printf("\n");
    }
    return 0;
}