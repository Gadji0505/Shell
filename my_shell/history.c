#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_FILE "history.txt"
#define MAX_INPUT_SIZE 1024
#define MAX_HISTORY_SIZE 100

char *history[MAX_HISTORY_SIZE];
int history_count = 0;

void load_history() {
    FILE *file = fopen(HISTORY_FILE, "r");
    if (file) {
        char line[MAX_INPUT_SIZE];
        while (fgets(line, sizeof(line), file) && history_count < MAX_HISTORY_SIZE) {
            line[strcspn(line, "\n")] = 0; // Удаление символа новой строки
            history[history_count] = strdup(line);
            history_count++;
        }
        fclose(file);
    }
}

void save_history() {
    FILE *file = fopen(HISTORY_FILE, "w");
    if (file) {
        for (int i = 0; i < history_count; i++) {
            fprintf(file, "%s\n", history[i]);
            free(history[i]); // Освобождение памяти
        }
        fclose(file);
    }
}

void add_to_history(const char *command) {
    if (history_count < MAX_HISTORY_SIZE) {
        history[history_count] = strdup(command);
        history_count++;
    } else {
        free(history[0]); // Удаление самого старого
        for (int i = 1; i < MAX_HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[MAX_HISTORY_SIZE - 1] = strdup(command);
    }
}

void display_history() {
    printf("История команд:\n");
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

