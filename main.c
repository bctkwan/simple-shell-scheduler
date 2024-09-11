/*
 * CSCI3150
 * Programming Assignment 1
 * Deliverable 1 - Simple Shell
 * 
 * Kwan Chun Tat
 * 1155033423
 * 
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/types.h>

int terminate_shell(int exitCode) {
	printf("The OS shell has terminated.\n");
	exit(exitCode);
	}

int get_current_working_dir(char* cwd) {
	if (getcwd(cwd, PATH_MAX+1) == NULL) {
		printf("[3150 shell]: cannot get current working directory\n");
		terminate_shell(-1);
	}
	return 0;
}

int get_input(char* input) {
	if (fgets(input, 257, stdin) == NULL) {
		putchar('\n');
		terminate_shell(0);
	}
	input[strlen(input)-1] = '\0';
	return 0;
}

char** tokenize_input(char* input) {
	char **argList = (char**) malloc(sizeof(char*));
	char *token = strtok(input, " ");
	argList[0] = (char*) malloc(sizeof(char) * strlen(token));
	strcpy(argList[0], token);
	token = strtok(NULL, " ");
	int i;
	for (i = 1; token != NULL; i++) {
		argList = (char**) realloc(argList, sizeof(char*) * (i +1));
		if (argList == NULL) {
			printf("[3150 shell]: fail to reallocate memory\n");
			terminate_shell(-1);
		}
		argList[i] = (char*) malloc(sizeof(char) * strlen(token));
		strcpy(argList[i], token);
		token = strtok(NULL, " ");
	}
	argList[i] = NULL;
	return argList;
}

glob_t expand_argument(char** argList) {
	glob_t globbuf;
	globbuf.gl_offs = 1;
	glob(argList[1], GLOB_DOOFFS | GLOB_NOCHECK, NULL, &globbuf);
	for (int i = 2; argList[i] != NULL; i++) {
		glob(argList[i], GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &globbuf);
	}
	globbuf.gl_pathv[0] = argList[0];
	return globbuf;
}

int main(void) {
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	char cwd[PATH_MAX+1];
	char input[257];
	
	while (1) {
		get_current_working_dir(cwd);
		printf ("[3150 shell:%s]$ ",cwd);
		get_input(input);
		if (strlen(input) == 0) {
			continue;
		}
		char** argList = tokenize_input(input);
		if (argList[1] != NULL) {
			glob_t globbuf = expand_argument(argList);
			argList = globbuf.gl_pathv;
		}
		
		if (strcmp(argList[0], "cd") == 0) {
			if (argList[1] != NULL && argList[2] == NULL) {
				if (chdir(argList[1]) == -1) {
					printf("%s: cannot change directory\n", argList[1]);
				}
			}
			else
				printf("cd: wrong number of arguments\n");
			continue;
		}

		if (strcmp(argList[0], "exit") == 0) {
			if (argList[1] == NULL) {
				terminate_shell(0);
			}
			else {
				printf("exit: wrong number of arguments\n");
				continue;
			}
		}
		
		pid_t child_pid;
		if (!(child_pid = fork())) {
			signal(SIGINT,SIG_DFL);
			signal(SIGQUIT,SIG_DFL);
			signal(SIGTERM,SIG_DFL);
			signal(SIGTSTP,SIG_DFL);
			setenv("PATH", "/bin:/usr/bin:.", 1);
			execvp(*argList,argList);
			if (errno == ENOENT) {
				printf("%s: command not found\n", argList[0]);
				continue;
			}
			else {
				printf("%s: unknown error\n", argList[0]);
			}
		}
		else {
			wait(NULL);
		}
	}

		
	return 0;
}

