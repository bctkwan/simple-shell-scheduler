/*
 * CSCI3150
 * Programming Assignment 1
 * Deliverable 2 - Simple Scheduler
 * 
 * Kwan Chun Tat
 * 1155033423
 * 
 */

#include <glob.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char** tokenize_input(char* input) {
	char **argList = (char**) malloc(sizeof(char*));
	char *token = strtok(input, " ");
	argList[0] = (char*) malloc(strlen(token) + 1);
	strcpy(argList[0], token);
	token = strtok(NULL, " ");
	int i;
	for (i = 1; token != NULL; i++) {
		argList = (char**) realloc(argList, sizeof(char*) * (i +1));
		argList[i] = (char*) malloc(strlen(token) + 1);
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

pid_t child_pid;

void alarmHandlerFIFO(int signal) {
	kill(child_pid, SIGTERM);
}

pid_t pids[10][2];

void alarmHandlerPARA(int signal) {
	pid_t myPid = getpid();
	int i;
	for (i = 0; pids[i][0] != myPid; i++);
	kill(pids[i][1], SIGTERM);
}

int main(int argc, char **argv)
{
	char input[10][512];
	FILE *file = fopen(argv[2], "r");
	int i = 0;
	while (i < 10 && fgets(input[i], 512, file) != NULL) {
		input[i][strlen(input[i])-1] = '\0';
		if (strlen(input[i]) == 0) continue;
		i++;
	}
	
	char command[10][512];
	int duration[10];
	char *token;
	for (int j = 0; j < i; j++) {
		token = strtok(input[j], "\t");
		strcpy(command[j], token);
		token = strtok(NULL, "\t");
		duration[j] = atoi(token);
	}
	
	char** argList[10];
	for (int j = 0; j < i; j++) {
		argList[j] = tokenize_input(command[j]);
		if (argList[j][1] != NULL) {
			glob_t globbuf = expand_argument(argList[j]);
			argList[j] = globbuf.gl_pathv;
		}
	}
	
	if (strcmp(argv[1], "FIFO") == 0) {
		signal(SIGALRM, alarmHandlerFIFO);
		clock_t startTime, endTime;
		struct tms cpuTime;
		double ticks_per_sec = (double) sysconf(_SC_CLK_TCK);
		for (int j = 0; j < i; j++) {
			if (!(child_pid = fork())) {
				setenv("PATH", "/bin:/usr/bin:.", 1);
				execvp(*argList[j],argList[j]);
			}
			else {
				startTime = times(&cpuTime);
				if (duration[j] != -1) alarm(duration[j]);
				wait(NULL);
				endTime = times(&cpuTime);
				printf("<<Process %d>>\n", child_pid);
				printf("time elapsed: %.4f\n", (endTime - startTime) / ticks_per_sec);
				printf("user time   : %.4f\n", cpuTime.tms_cutime/ticks_per_sec);
				printf("system time : %.4f\n", cpuTime.tms_cstime/ticks_per_sec);
			}
		}
	}
	
	if (strcmp(argv[1], "PARA") == 0) {
		for (int j = 0; j < i; j++) {
			if (!(pids[j][0] = fork())) {
				pids[j][0] = getpid();
				signal(SIGALRM, alarmHandlerPARA);
				clock_t startTime, endTime;
				struct tms cpuTime;
				double ticks_per_sec = (double) sysconf(_SC_CLK_TCK);
				if (!(pids[j][1] = fork())) {
					pids[j][1] = getpid();
					setenv("PATH", "/bin:/usr/bin:.", 1);
					execvp(*argList[j],argList[j]);
				}
				else {
					startTime = times(&cpuTime);
					if (duration[j] != -1) alarm(duration[j]);
					wait(NULL);
					endTime = times(&cpuTime);
					printf("<<Process %d>>\n", pids[j][1]);
					printf("time elapsed: %.4f\n", (endTime - startTime) / ticks_per_sec);
					printf("user time   : %.4f\n", cpuTime.tms_cutime/ticks_per_sec);
					printf("system time : %.4f\n", cpuTime.tms_cstime/ticks_per_sec);
					exit(0);
				}
			}
		}
		for (int j = 0; j < i; j++) {
			waitpid(pids[j][0], NULL, 0);
		}
	}
	
	return 0;
}

