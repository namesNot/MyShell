#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#include "RecordError.h"
#include "StructProgForRun.h"
#include "RunCmd.h"
#include "ConstForTypeCmd.h"

enum ConveyorParts {FirstElem = 1, MiddleElem = 2, LastElem = 3};

static char ** CreateArgs(struct Cmd *ptr, int *cc)
{
	int count = 0;
	struct Cmd *tmp = ptr;
	char **arr_args = NULL;
	while(tmp){
		count++;
		tmp = tmp->next;
	}
	arr_args = malloc(sizeof(char **) * (count +1));
	IfErrorMalloc(arr_args);

	for(int i=0;ptr;ptr=ptr->next,i++)
		arr_args[i] = ptr->word;
	
	arr_args[count] = NULL;
	*cc = count;

	return arr_args;
}

static int RedirectStream(struct TableRedirect *table)
{
	if(table->fd_streams[0] == 1){
		int fd = open(table->filename[0], O_RDONLY);
		if(fd == -1){
			perror(table->filename[0]);
			return -1;
		}	
		dup2(fd,0);
		close(fd);
		table->fd_streams[0] = 0;
		free(table->filename[0]);
		table->filename[0] = NULL;
	}
	if(table->fd_streams[1] == 1){
		int fd;
		if(table->add_in_file){
			fd = open(table->filename[1], O_WRONLY | O_CREAT | O_APPEND, 0666);
		}
		else
			fd = open(table->filename[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(fd == -1){
			perror(table->filename[1]);
			return -1;
		}	
		dup2(fd,1);
		close(fd);
		table->fd_streams[1] = 0;
		free(table->filename[1]);
		table->filename[1] = NULL;

	}
	return 0;
}

static void CreateOrAddInGroup(int *pgid)
{
	if(*pgid == 0){
		if(setpgid(getpid(), 0) != 0){ //setpgid переводит процесс в другую группу, ТЕРЯЯ управляющий терминал.
			RecordError(NULL);     // Процесс становится лидером группы
		}
	}
	else{
		if(setpgid(getpid(),*pgid) != 0){ //setpgid переводит процесс в другую группу
			RecordError(NULL);     // 
		}
	}
}

static void SettingDescriptorsForConveyor(enum ConveyorParts pos, int *fd, int *fd2)
{
	if(pos == FirstElem){
			dup2(fd[1],1);
			close(fd[1]);
			close(fd[0]);
		}
	if(pos == MiddleElem){
		dup2(fd[0],0);
		dup2(fd2[1],1);
		close(fd[1]);
		close(fd[0]);
		close(fd2[0]);
		close(fd2[1]);
	}
	if(pos == LastElem){
		dup2(fd[0],0);	
		close(fd[1]);
		close(fd[0]);
	}
}


static int RunProcess(struct ProgForRun *arr_progs, int i, int conv, int *fd, int *fd2, enum ConveyorParts pos, int *pgid)
{
	char **args = NULL;
	int pid_child, count = 0;

	args = CreateArgs(arr_progs->command[i],&count);
	if(args[0] != NULL && strcmp(args[0],"cd") == 0){
		int res;
		if(args[1] == NULL)
			res = chdir(getenv("HOME"));
		else
			res = chdir(args[1]);
		if(res == -1){
			perror(args[0]);		
		}
		free(args);
		return -1;
	}

	if(strcmp(args[count-1],"&") == 0){
		args[count-1] = NULL;
	}


	if((pid_child = fork()) == 0){

	//tcsetattr(0, TCSANOW, &ts2);
				
		CreateOrAddInGroup(pgid);
		close(4);
		if(arr_progs->table[i] != NULL){
			RedirectStream(arr_progs->table[i]);
		}

		if(conv){
			SettingDescriptorsForConveyor(pos, fd, fd2);
		}
		execvp(args[0], args);
		perror(args[0]);
		exit(2);
	}
	if(*pgid == 0){
		*pgid = pid_child;
	}	
	if(args[count-1] == NULL){//обращаем внимание только на & в последней команде в конвейере
		pid_child = -1; 	  //тогда в фоновый режим
	}
	
	free(args);	


	return pid_child;
}

int* RunCmd(struct ProgForRun *arr_progs)
{
	int *fd =malloc(sizeof(int *) * 2);
	IfErrorMalloc(fd);

	int conv = 0;
	int *ptr_pid = malloc(sizeof(int) * arr_progs->count);
	IfErrorMalloc(ptr_pid);

	int background_mode = 0;
	int pgid = 0;


	for(int i=0; i < arr_progs->count; i++){

		if(i+1 < (arr_progs->count) && arr_progs->type_cmd[i+1] == PART_OF_CONVEYOR && conv == 0){
			pipe(fd);
			conv = 1;
		}
		if(i==0){
			ptr_pid[i] = RunProcess(arr_progs, i, conv, fd, NULL, FirstElem, &pgid);
			background_mode = (ptr_pid[i] == -1 ? 1: background_mode);
			continue;
			
		}
		if(i == (arr_progs->count - 1)){
			ptr_pid[i] = RunProcess(arr_progs, i, conv, fd, NULL, LastElem, &pgid);
			background_mode = (ptr_pid[i] == -1 ? 1: background_mode);
			continue;
		}


		int *fd2 = malloc(sizeof(int *) * 2);
		pipe(fd2);

		ptr_pid[i] = RunProcess(arr_progs, i, conv, fd, fd2, MiddleElem, &pgid);
		background_mode = (ptr_pid[i] == -1 ? 1: background_mode);

		close(fd[0]);
		close(fd[1]);
		fd[0] = fd2[0];
		fd[1] = fd2[1];
		free(fd2);
		


	}
	if(conv){
		close(fd[0]);
		close(fd[1]);
	}

	free(fd);
	
	if(background_mode){
		free(ptr_pid);
		ptr_pid = NULL;
	}
	else{
		tcsetpgrp(STDIN_FILENO,pgid); // передаем управл. терм. только если процесс не запускается как фоновый &
		kill(pgid, SIGCONT);  // В случае если дочерний процесс начнется раньше, он преостановиться из-за сигнала 
				     // SIGTTIN/SIGTTOU разбудим его отправкой сигнала SIGCONT.
		printf("%d\n", pgid);
	}



	return ptr_pid;

}
