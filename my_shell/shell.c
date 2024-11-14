#include "shell.h"
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024

void signal_handler(int signo) {
    if (signo == SIGHUP) {
        printf("Configuration reloaded\n");
    }
}

void display_help() {
    printf("Доступные команды:\n");
    printf("  exit или \\q - выход из шелла\n");
    printf("  echo <текст> - выводит текст\n");
    printf("  \\e <переменная> - выводит значение переменной окружения\n");
    printf("  \\l /dev/sda - выводит информацию о разделе\n");
    printf("  \\v - выводит список процессов\n");
    printf("  history - показывает историю команд\n");
    printf("  help - выводит это сообщение\n");
    printf("  <путь к исполняемому файлу> - выполняет указанный бинарный файл\n");
}

void execute_command(char *command) {
    if (strcmp(command, "exit") == 0 || strcmp(command, "\\q") == 0) {
        exit(0);
    } else if (strcmp(command, "help") == 0) {
        display_help();
    } else if (strcmp(command, "history") == 0) {
        display_history();
    } else if (strncmp(command, "echo ", 5) == 0) {
        printf("%s\n", command + 5);
    } else if (strncmp(command, "\\e ", 3) == 0) {
        char *var = getenv(command + 3);
        if (var) {
            printf("%s\n", var);
        } else {
            printf("Переменная не найдена\n");
        }
    } else if (strcmp(command, "\\l /dev/sda") == 0) {
        system("lsblk");
    } else if (strcmp(command, "\\v") == 0) {
        system("ps -e");
    } else if (strncmp(command, "\\mount ", 7) == 0) {
        // Подключение VFS
        system("mkdir -p /tmp/vfs");
        system("mount -t tmpfs tmpfs /tmp/vfs");
        system("ps -e > /tmp/vfs/task_list.txt");
        printf("VFS подключен в /tmp/vfs и список задач сохранен в task_list.txt\n");
    } else if (strncmp(command, "\\dump ", 6) == 0) {
        pid_t pid = atoi(command + 6);
        char dump_command[128];
        snprintf(dump_command, sizeof(dump_command), "gcore %d", pid);
        system(dump_command);
    } else {
        // Выполнение указанного бинарного файла
        if (access(command, X_OK) == 0) {
            system(command);
        } else {
            printf("Файл не найден или не является исполняемым: %s\n", command);
        }
    }
}

void start_shell() {
    char input[MAX_INPUT_SIZE];
    signal(SIGHUP, signal_handler);
    load_history();

    while (1) {
        printf("my_shell> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // Ctrl+D
        }

        // Удаление символа новой строки
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) > 0) {
            add_to_history(input);
            execute_command(input);
        }
    }

    save_history();
}

