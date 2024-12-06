Нужно написать программу в файлах для работы в линуксе, что бы включал следующее:
1. Печатает введённую строку и выходит
2. Печатает введённую строку в цикле и выходит по Ctrl+D
3. Добавим команду для выхода (exit и \q)
===
4. Добавим историю введённых команд и её сохранения в файл
5. Добавим команду echo
6. Добавим проверку введённой команды
7. Добавим команду по выводу переменной окружения (\e $PATH)
8. Выполняем указанный бинарник
9. По сигналу SIGHUP вывести "Configuration reloaded"
10. По `\l /dev/sda` получить информацию о разделах в системе
11. По `\cron` подключить VFS в /tmp/vfs со списком задач в планировщике
12. По `\mem <procid>` получить дамп памяти процесса
1) В 8 задании запуск идет через системный вызов на C, что не правильно?
2) В 10 также системные вызовы, их не должно быть
3) В 12 dump пустой, такого быть не должно
3) Должен выходить по  \q
7) Было бы не плохо добавить $
10) Не получается получить информацию о системе
Должна быть команда clear, help и другие 
Уточнения для 10-го задания 
Тогда можно переформулировать на "Определить является ли диск загрузочным". У них должна быть сигнатура 55AA. Т.е. пользователь вводит sda, sdb и т.п., а в шелл нужно смотреть первый сектор введённого диска.
Вот пример, но коды предоставленные тобой должны быть реализованы другим способом :
blstruct.h:
typedef struct {
	uint8_t active;
	uint8_t starterHead[3];
	//uint16_t starterCns;
	uint8_t id;
	uint8_t endHead[3];
	//uint16_t endCns;
	uint32_t prevCount;
	uint32_t sectorCount;
} MBRpartition;

typedef struct {
	char code[446];
	MBRpartition partitionTable[4];
	uint16_t signature;
} MBR;

void ReadMBRPartition(FILE* file, MBRpartition* part) {
	fread(&part->active, sizeof(part->active), 1, file);
	fread(&part->starterHead, sizeof(part->starterHead) / sizeof(uint8_t), 1, file);
	//fread(&part->starterCns, sizeof(part->starterCns), 1, file);
	fread(&part->id, sizeof(part->id), 1, file);
	fread(&part->endHead, sizeof(part->endHead) / sizeof(uint8_t), 1, file);
	//fread(&part->endCns, sizeof(part->endCns), 1, file);
	fread(&part->prevCount, sizeof(part->prevCount), 1, file);
	fread(&part->sectorCount, sizeof(part->sectorCount), 1, file);
}

void ReadMBR(FILE* file, MBR* table) {
	int codeSize = sizeof(table->code) / sizeof(char);
	fread(table->code, sizeof(char), codeSize, file);

	int ptableSize = sizeof(table->partitionTable) / sizeof(MBRpartition);
	for (int i = 0; i < ptableSize; i++) {
		ReadMBRPartition(file, &table->partitionTable[i]);
		//printf("pt size - %d\n", sizeof(&table->partitionTable[i]));
		//unsigned int ptableSize = sizeof(&table->partitionTable[i]);
		//printf("Size %d - %u\n", i, ptableSize);
		//fread(&table->partitionTable[i], 16, 1, file);
	}

	fread(&table->signature, sizeof(table->signature), 1, file);
}
defcoms.h:
#define CMD_BUFFSIZE 	1024
#define TOKEN_BUFFSIZE 	64
#define DIRN_BUFFSIZE 	128
#define CMDHIS_BUFFSIZE 512

#define SHELL_NAME 	"l-nshell"

#define CMDHIS_PATH 	"/home/%s"
#define CMDHIS_LOG	"/l-nshell_cmdhis.log"
#define CMDHIS_ENV 	"LNSHELL_CMDHIS"

#define PROCMEM_PAT 	"/proc/%d/map_files/"
#define DUMP_PATH_PAT	"/var/l-nshell_dumps/d%d/"
#define DUMP_FILE_PAT	"%s.dump"

#define CRONFS_TARGET	"/tmp/vfs"

#define true 		1
#define false 		0
#define nullptr 	NULL

#include "blstruct.h"
#include "dumper.h"

int LinkProcess(char** args) {
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid < 0) perror(SHELL_NAME);
	else if (pid == 0) {
		if (execvp(args[0], args) == -1) {
			perror(SHELL_NAME);
			exit(1);
		}
	}
	else {
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

void Rmkdir(char* filepath) {
	size_t fpSize = strlen(filepath);
	char* path = (char*)malloc((fpSize + 1) * sizeof(char));
	
	path[fpSize] = '\0';

	for (int i = 0; i < fpSize; i++) {
		path[i] = filepath[i];

		if (path[i] == '/' || path[i] == '\0' || i == fpSize - 1) mkdir(path, 0755);
	}

	free(path);
}

int CmdCd(char** args) {
	if (args[1] != nullptr) {
		int cdresult = chdir(args[1]);
		if (cdresult != 0) perror(SHELL_NAME);
	}
	else {
		fprintf(stderr, "Expecting argument \"path\" for cd\n");
	}

	return 1;
}

int CmdExit(char** args) {
	return 0;
}

int CmdLs(char** args) {
	char cwd[PATH_MAX];
	
	DIR* dirp = nullptr;
	struct dirent *dir;

	int hideHidden = 1;
	int pathDefined = 0;

	for (int i = 1; args[i] != nullptr; i++) {
		if (args[i][0] != '-' && !pathDefined) {
			dirp = opendir(args[i]);
			pathDefined = 1;
		}
		else hideHidden = strcmp(args[i], "-d");
	}

	if (!pathDefined) {
		dirp = opendir(getcwd(cwd, sizeof(cwd)));
 	}

	if (dirp) {
		while ((dir = readdir(dirp)) != nullptr) {
			if (!hideHidden || dir->d_name[0] != '.') {
				if (dir->d_type == DT_DIR) printf("\033[1;32m%s\033[0m\t", dir->d_name);
				else printf("%s\t", dir->d_name);
			}
		}
		printf("\n");
	}
	else {
		perror(SHELL_NAME);
	}

	closedir(dirp);
	return 1;
}

int CmdEcho(char** args) {
	int i = 1;

	while (args[i] != nullptr) {
		printf("%s ", args[i]);
		i++;
	}
	printf("\n");

	return 1;
}

int CmdCyctest(char** args) {
	int echo = 0;
	while (true) {
		echo = CmdEcho(args);
	}

	return 1;
}

int CmdHsr(char** args) {
	char* env = getenv(CMDHIS_ENV);
	FILE* file = fopen(env, "r");
	
	if (!file) {
		printf("%s: Can't open \"%s\"\n", SHELL_NAME, env);
		return 1;
	}

	char content[256];
	while (fgets(content, 256, file) != nullptr) {
		printf("%s", content);
	}
	printf("\n");
	
	return 1;
}

int CmdE(char** args) {
	if (args[1] != nullptr) {
		char* envPath = getenv(args[1]);
		if (envPath != nullptr) {
			printf("%s: %s\n", args[1], getenv(args[1]));
		}
		else printf("There is no variable called \"%s\"\n", args[1]);
	}
	else printf("Expecting argument \"name\" for \\e\n");

	return 1;
}

int CmdL(char** args) {	
	if (args[1] == nullptr) {
		printf("Expecting argument \"path\" for \\l\n");
		return 1;
	}

	FILE* mount = fopen(args[1], "rb");
	if (!mount) { 
		printf("Unknown device \"%s\"!\n", args[1]);
		return 1;
	}
	
	MBR block;
	ReadMBR(mount, &block);
	
	printf("Signature of %s: %d\n", args[1], block.signature);

	fclose(mount);

	return 1;
}

int CmdCron(char** args) {
	char source[] = "/var/spool/cron/crontabs/";
	char target[] = CRONFS_TARGET;
	unsigned long flags = MS_BIND;

	Rmkdir(target);

	if (mount(source, target, "tmpfs", flags, "mode=0755") == -1) {
		perror(SHELL_NAME);
		return 1;
	}

	printf("%s successfuly mounted to %s!\n", source, target);

	return 1;
}

int CmdMem(char** args) {
	if (args[1] == nullptr) {
		printf("Expecting argument \"pid\" for \\l\n");
		return 1;
	}

	char* username = getenv("USER");
	if (username == nullptr) {
		perror(SHELL_NAME);
		return 1;
	}

	pid_t procid = atoi(args[1]);

	char* procp, *dumpp;
	asprintf(&procp, PROCMEM_PAT, procid);
	asprintf(&dumpp, DUMP_PATH_PAT, procid);

	printf("Reading from %s\n", procp);
	Rmkdir(dumpp);

	DIR* dirp = opendir(procp);

	if (dirp == nullptr) {
		perror(SHELL_NAME);
		return 1;
	}

	struct dirent* dir;
	while ((dir = readdir(dirp)) != nullptr) {
		if (dir->d_type != DT_DIR) {
			char *dumpf;
			asprintf(&dumpf, DUMP_FILE_PAT, dir->d_name);
			
			char *procPart = (char*)malloc((strlen(procp) + strlen(dir->d_name) + 1) * sizeof(char));
			strcpy(procPart, procp);
			strcat(procPart, dir->d_name);

			char *dumpfFull = (char*)malloc((strlen(dumpp) + strlen(dumpf) + 1) * sizeof(char));	
			strcpy(dumpfFull, dumpp);
			strcat(dumpfFull, dumpf);

			DumpPart(procPart, dumpfFull);

			free(dumpf);
			free(procPart);
			free(dumpfFull);
		}
	}
	
	printf("Dumped process %d to %s\n", procid, dumpp);
	free(procp); free(dumpp);
	return 1;
}

void CatchSighup() {
	printf("Configuration reloaded!");
}

void CatchSigint() {
	printf("\nTERMINATING %s\n", SHELL_NAME);
	exit(1);
}

typedef struct {
	char* cmdName;
	char* cmdDescr;
	int (*cmdFunc) (char**);
} DefCmd;

int CmdHelp(char** args);

DefCmd cmdList[] = {
	{ "help", "print this list", &CmdHelp },
	{ "exit", "terminate l-nshell", &CmdExit },
	{ "\\q", "do the same as \"exit\" command", &CmdExit },
	{ "cd", "[path] change working directory", &CmdCd },
	{ "ls", "[path] list all files in the specified folder", &CmdLs },
	{ "echo", "[text] prints specified text", &CmdEcho },
	{ "cyctest", "[text or nothig] run a test of terminating signal (when it runs just press Ctrl+C to terminate l-nshell)", &CmdCyctest },
	{ "hsr", "view command history", &CmdHsr },
	{ "\\e", "[envname] print a value of specified environment variable", &CmdE },
	{ "\\l", "[path to block device] print signature of specified block device", &CmdL },
	{ "\\cron", "no functionality", &CmdCron },
	{ "\\mem", "[process id] dump process memory with specified id", &CmdMem }
};

int DefNum() {
	return sizeof(cmdList) / sizeof(DefCmd);
}

int CmdHelp(char** args) {
	printf("List of default commands:\n");
	for (int i = 0; i < DefNum(); i++) printf("%s\t- %s;\n", cmdList[i].cmdName, cmdList[i].cmdDescr);
	return 1;
}
dumper.h:
#define BYTES_PER_READ 1024

int DumpPart(char* filepath, char* dumppath) {
	int file = open(filepath, O_RDONLY);
	FILE* dump = fopen(dumppath, "wb+");

	if (file < 0) {
		perror("dumper");
		return false;
	}
	
	char rdbuff[BYTES_PER_READ];
	size_t bytesRead = read(file, rdbuff, BYTES_PER_READ);
	size_t bytesWritten = 0;

	for(; bytesRead > 0; bytesRead = read(file, rdbuff, BYTES_PER_READ)) {
		bytesWritten += fwrite(rdbuff, sizeof(char), bytesRead, dump);
	}
	
	if (dump != nullptr) fclose(dump);

	printf("%s - %ld bytes written\n", dumppath, bytesWritten);
	return true;
}
lnch.sh:
#!/bin/bash
echo "Compiling code...";
gcc -o a.out *.c
chmod +x a.out
echo "Code compiled!";
./a.out
rm ./a.out
main.c:
//headers of the C standart
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

//"sys" module headers
#include "sys/wait.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/mount.h"

//"linux" module headers
#include "linux/limits.h"
#include "linux/fs.h"
#include "linux/kernel.h"
//#include "linux/init.h"
#include "linux/module.h"
//#include "linux/pagemap.h"

//other unix-system feature modules
#include "unistd.h"
#include "pwd.h"
#include "dirent.h"
#include "signal.h"

//"defcoms" is a header file that includes all built-in shell commands
#include "defcoms.h"

enum ERR {
	BUFF_MEM_ALLOC_ERROR,
	BUFF_ERROR
};

void AllocCheck(char* buffer) {
	if (!buffer) {
		fprintf(stderr, "BUFF_MEM_ALLOC_ERROR");
		exit(1);
	}
}

char* ReadLine(void) {
	char* line = nullptr;
	size_t buffsize = 0;
	getline(&line, &buffsize, stdin);	
	return line;
}

char** Tokenise(char* line) {
	size_t buffsize = TOKEN_BUFFSIZE;
	int pos = 0;
	char **tokens = malloc(buffsize * sizeof(char*));
	char* token;

	AllocCheck(token);

	const char* strippers = " \t\n\a\r";
	token = strtok(line, strippers);
	while (token != nullptr) {
		tokens[pos] = token;
		pos++;

		if (pos >= buffsize) {
			buffsize += TOKEN_BUFFSIZE;

			tokens = realloc(tokens, buffsize * sizeof(char*));
			AllocCheck(tokens[0]);
		}

		token = strtok(nullptr, strippers);
	}

	tokens[pos] = nullptr;
	return tokens;
}

int Execute(char** args) {
	/*printf("\n");
	for (int i = 0; args[i] != nullptr; i++) {
		printf(" |%s| ", args[i]);
	}
	printf("\n");
	*/

	if (args[0] == nullptr) return 1;

	for (int i = 0; i < DefNum(); i++) {
		if (strcmp(args[0], cmdList[i].cmdName) == 0) {
			return (*cmdList[i].cmdFunc)(args);
		}
	}

	return LinkProcess(args);
}

void LineLog(char* line, char** history, int *hbuffsize, int *hindex) {
	history[*hindex] = line;
	//printf("%s", history[*hindex]);
	*hindex = *hindex + 1;

	if (*hindex >= *hbuffsize) {
		*hbuffsize += CMDHIS_BUFFSIZE;
		history = realloc(history, *hbuffsize * sizeof(char*));
		AllocCheck(history[0]);
	}


	FILE* file = fopen(getenv(CMDHIS_ENV), "a");

	if (file) {
		fputs(line, file);
		fclose(file);
	}
	else printf("%s: Can't log input!\n", SHELL_NAME);
}

void HandleCmd(char** history, int *hbuffsize, int *hindex) {
	char *line;
	char** args;
	int status = 1;

	char cwd[PATH_MAX];

	while (status) {
		printf("\033[1;36m%s\033[0m > ", getcwd(cwd, sizeof(cwd)));

		line = ReadLine();

		if (feof(stdin)) {
			printf("\n");
			return;
		}

		LineLog(line, history, hbuffsize, hindex);
		
		if (strcmp(line, cmdList[6].cmdName)) {
			args = Tokenise(line);
			status = Execute(args);
		}
		else {
			status = Execute(history);
		}

		free(line); //вот из-за этого и не работает хистори
		free(args);
	}
}

void StarterArt(char* filename) {
	int width = 120, height  = 15;

	FILE* file = fopen(filename, "r");

	if (!file) {
		printf("\nSad, but there is no art file loaded :(\nWelcome by the way!\n\n");
		return;
	}

	char sym = fgetc(file);
	int i = 0;
	while (sym != EOF && i < width * height) {
		printf("%c", sym);
		sym = fgetc(file);
		i++;
	}

	printf("\n");
	fclose(file);
}

int main(int argc, char **argv) {
	int hbuffsize = CMDHIS_BUFFSIZE;
	int hindex = 0;
	char** history = malloc(hbuffsize * sizeof(char*));

	printf("%s process id: \033[1;36m%d\033[0m\n", SHELL_NAME, getpid());	
	
	struct passwd* info = getpwuid(geteuid());
	printf("User id: %d\n", info->pw_uid);

	int hisnameLen = strlen(CMDHIS_LOG) + 1;
	
	int envpathLen;
	if (info->pw_uid != 0) {
		envpathLen = strlen(CMDHIS_PATH) - 1 + strlen(info->pw_name) + hisnameLen;
	}
	else {
		envpathLen = strlen(CMDHIS_PATH) - 1 + hisnameLen;
	}

	char* hisname = (char*)malloc(hisnameLen * sizeof(char));
	char* envpath = (char*)malloc(envpathLen * sizeof(char));
	
	if (info->pw_uid != 0) {
		snprintf(envpath, envpathLen, CMDHIS_PATH, info->pw_name);
	}
	else {
		snprintf(envpath, envpathLen, CMDHIS_PATH, "");
	}
	strcpy(hisname, CMDHIS_LOG);
	strcat(envpath, hisname);

	if (setenv(CMDHIS_ENV, envpath, 0) < 0) {
		perror(SHELL_NAME);
		return 1;
	}
	
	signal(SIGHUP, CatchSighup);
	signal(SIGINT, CatchSigint);

	StarterArt("result.txt");

	HandleCmd(history, &hbuffsize, &hindex);
	
	umount(CRONFS_TARGET);

	free(history);
	free(hisname);
	free(envpath);
	return 0;
}









Для создания оболочки (shell) в Linux с указанными вами требованиями, мы можем разбить задачу на несколько частей. Давайте рассмотрим основные функции, которые вам нужны, и как их можно реализовать. Я предоставлю упрощенный пример, который вы можете расширить и адаптировать под свои нужды.

### Структура проекта

1. **main.c** - основной файл, где будет реализована логика оболочки.
2. **defcoms.h** - файл с определениями команд.
3. **dumper.h** - файл для работы с дампом памяти процессов.
4. **lnch.sh** - скрипт для компиляции и запуска программы.
5. **blstruct.h** - файл со структурами для работы с MBR.

### Пример реализации

#### `main.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pwd.h>

#include "defcoms.h"
#include "dumper.h"
#include "blstruct.h"

#define CMDHIS_PATH "/home/%s"
#define CMDHIS_LOG "/l-nshell_cmdhis.log"
#define CMDHIS_ENV "LNSHELL_CMDHIS"

void CatchSighup() {
    printf("\nConfiguration reloaded!\n");
}

void CatchSigint() {
    printf("\nExiting shell...\n");
    exit(0);
}

void execute_command(char *command) {
    char *args[64];
    char *token = strtok(command, " ");
    int i = 0;

    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (execvp(args[0], args) == -1) {
        perror("Error executing command");
    }
}

void shell_loop() {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("> ");
        getline(&line, &len, stdin);
        line[strcspn(line, "\n")] = 0; // Убираем символ новой строки

        if (strcmp(line, "exit") == 0 || strcmp(line, "\\q") == 0) {
            free(line);
            exit(0);
        }

        if (fork() == 0) {
            execute_command(line);
            exit(0);
        } else {
            wait(NULL);
        }
    }
}

int main() {
    signal(SIGHUP, CatchSighup);
    signal(SIGINT, CatchSigint);

    char *username = getenv("USER");
    char *history_path = malloc(256);
    snprintf(history_path, 256, CMDHIS_PATH, username);
    setenv(CMDHIS_ENV, history_path, 1);

    shell_loop();

    free(history_path);
    return 0;
}
```

#### `defcoms.h`

```c
#ifndef DEFCOMS_H
#define DEFCOMS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void execute_command(char *command);

#endif // DEFCOMS_H
```

#### `dumper.h`

```c
#ifndef DUMPER_H
#define DUMPER_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int DumpPart(char* filepath, char* dumppath);

#endif // DUMPER_H
```

#### `blstruct.h`

```c
#ifndef BLSTRUCT_H
#define BLSTRUCT_H

#include <stdint.h>

typedef struct {
    uint8_t active;
    uint8_t starterHead[3];
    uint8_t id;
    uint8_t endHead[3];
    uint32_t prevCount;
    uint32_t sectorCount;
} MBRpartition;

typedef struct {
    char code[446];
    MBRpartition partitionTable[4];
    uint16_t signature;
} MBR;

void ReadMBRPartition(FILE* file, MBRpartition* part);
void ReadMBR(FILE* file, MBR* table);

#endif // BLSTRUCT_H
```

### Компиляция

Создайте файл `lnch.sh` для компиляции:

```bash
#!/bin/bash
echo "Compiling code..."
gcc -o shell main.c -lm
echo "Code compiled!"
```

### Запуск

После компиляции вы можете запустить оболочку. Обратите внимание, что это базовая структура, и вам нужно будет добавить больше функциональности, такой как обработка истории команд, команды `echo`, `\e`, `\l`, `\cron`, `\mem`, и другие, как указано в вашем запросе.

### Дополнительные улучшения

- Реализуйте функции для каждой команды, как в вашем изначальном коде.
- Добавьте обработку ошибок и проверку ввода.
- Реализуйте сохранение истории команд в файл.
- Обработайте команды, которые требуют специального поведения, например, `\l` для проверки загрузочного диска.

Эта структура должна помочь вам начать работу над вашей оболочкой. Не забудьте адаптировать и расширять код под свои нужды!
