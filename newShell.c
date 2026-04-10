#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include "LineDiscipline.h"
#include "RecordError.h"
#include "ReturnValues.h"
#include "CheckSeparators.h"
#include "StructProgForRun.h"
#include "AnalisysString.h"
#include "FreeProgramForRun.h"
#include "RunCmd.h"
#include "ConstForTypeCmd.h"

#define READ_SIZE 100
#define SIZE_WORD 20
#define SIZE_COUNT_PATH 15



struct termios ts2;

void OpenErrorLog()
{
	int fd = open("error.log", O_WRONLY | O_APPEND); 
				
	if(fd == -1){
		RecordError("Ошибка выполнения программы. нет error.log\n");
		exit(1);
	}
	dup2(fd,4);
	close(fd);
}


void CreateStruct(struct ProgForRun *arr_progs)
{
	arr_progs->count = 0;
	arr_progs->size = 10;
	arr_progs->command = malloc(sizeof(struct Cmd **) * arr_progs->size);
	IfErrorMalloc(arr_progs->command);
	for(int i=0; i < arr_progs->size; i++)
		arr_progs->command[i] = NULL;

	arr_progs->table = malloc(sizeof(struct TableRedirect **) * arr_progs->size);
	IfErrorMalloc(arr_progs->table);
	for(int i=0; i < arr_progs->size; i++)
		arr_progs->table[i] = NULL;

	arr_progs->type_cmd = malloc(sizeof(enum ConstForTypeCmd *) * arr_progs->size);	
	IfErrorMalloc(arr_progs->type_cmd);
	for(int i=0; i < arr_progs->size; i++)
		arr_progs->type_cmd[i] = STANDART_CMD;
}


int AnalisysArrProgs(struct ProgForRun *arr_progs)
{
	for(int i=0;i<arr_progs->count; i++){
		if((i+1) < arr_progs->count){
			if(arr_progs->type_cmd[i+1] == 2){
				if(arr_progs->table[i] != NULL){
					if(arr_progs->table[i]->filename[1] != NULL){
						RecordError("error");
						return 0;
					}
				//другое допускается
				}
				if(arr_progs->table[i+1] != NULL){
					if(arr_progs->table[i+1]->filename[0] != NULL){
						RecordError("error");
						return 0;
					}
				}
			}
	
		}

	}
	return 1;
}

void handler(int s)
{
	signal(SIGCHLD, handler);
	int pid, status;
	while((pid = wait4(-1, &status, WNOHANG, NULL)) > 0){
		if(WIFSIGNALED(status)){
			psignal(WTERMSIG(status), NULL);
		}
		printf("pid SIGCHLD %d\n",pid);
		puts("Finished");
	}
}


char ** GetPATH()
{
	int count = 0, pos_new_word = -1;
	char *path = getenv("PATH");
	char **str_path = malloc(sizeof(char *) * SIZE_COUNT_PATH);
	IfErrorMalloc(str_path);
	
	for(int i=0; 1; i++){
		int j;
		if(pos_new_word == -1 && path[i] != ':'){ // получаем первую позицию нового слова
			pos_new_word = i;
		}
		if(path[i] == ':' || path[i] == '\0'){
			str_path[count] = malloc(sizeof(char) * (i - pos_new_word + 1)); // создаем подходящий массив для строки
			IfErrorMalloc(str_path);					// пути к каталогу
												
			j = 0;
			for(j=0;pos_new_word < i; pos_new_word++ ,j++){ // копируем символы из path в новый массив str_path
				str_path[count][j] = path[pos_new_word];
			}
			str_path[count][j] = '\0';
			pos_new_word = -1;
			count++;
			if(path[i] == '\0')
				break;
		}	

	}
	str_path[count] = NULL;
	
	return str_path;
}

void FreePathStr(char **str_path)
{
	for(int i=0; str_path[i]; i++){
		free(str_path[i]);
	}
	free(str_path);
	
}
int main(int argc, char **argv)
{
	int c,pid, status = 0;
	int *pid_arr = NULL;
	struct Cmd *tmp;
	
	OpenErrorLog();
	char **str_path = GetPATH();
	//проверять не буду. проверка делается внутри


	char *string = malloc(sizeof(char *) * READ_SIZE);
	IfErrorMalloc(string);
	
	struct ProgForRun arr_progs;

	CreateStruct(&arr_progs);
	
	write(1,"> ",2);
	int ready = 1;
	struct termios ts1;

	tcgetattr(0, &ts1);
	//struct termios ts2;
	tcgetattr(0, &ts2);
	ts1.c_lflag &= ~(ICANON | ECHO);
	//tcsetattr(0, TCSANOW, &ts1);



	while(ready){
		tcsetattr(0, TCSANOW, &ts1);		//Выкл ECHO
		c = AutoCompletion(string, str_path);
		tcsetattr(0, TCSANOW, &ts2);		//Вкл ECHO: Например для случая: cat > q.txt т.е. cat берет инфу с терм.

		if(c == ERROR_INPUT){
			//записать ошибку
			return 6;
		}
		else if(c == EXIT){
			break;
		}

		string[c-1] = '\0';

		AnalisysString(string, c, &arr_progs);
		/*if(conv.command[0] != NULL)
			pid = RunConveyer(&conv);*/
		for(int i=0;i<arr_progs.count;i++){
			puts("---------");
			tmp = arr_progs.command[i];
			while(tmp){
				printf("[%s] ",tmp->word);
				tmp = tmp->next;
				printf("\n");
			}
			if(arr_progs.table[i] != NULL){
				if(arr_progs.table[i]->filename[0])
					printf("in %s ",arr_progs.table[i]->filename[0]);
				if(arr_progs.table[i]->filename[1])
					printf("out %s ",arr_progs.table[i]->filename[1]);
				if(arr_progs.table[i]->filename[2])
					printf("err %s ",arr_progs.table[i]->filename[2]);
				puts("");
			}
			printf("mode %d\n",arr_progs.type_cmd[i]);
	}	
	if(AnalisysArrProgs(&arr_progs) == 0)
		FreeProgramForRun(&arr_progs);
	
	signal(SIGCHLD, SIG_DFL);
	

//  
	signal(SIGTTIN, SIG_DFL); // Устанавл. в Defolt, так как при игнориров. этих сигналов в дочерних процессах, при получении
	signal(SIGTTOU, SIG_DFL); // сигнала SIGTTOU/SIGTTIN - будем получать ошибку ввода/вывода и программа завершается.
				  // А при Defolt режиме, при получении сигнала, программа приостанавливается.
				  
	pid_arr = RunCmd(&arr_progs);
	signal(SIGTTIN, SIG_IGN); // Эти сигналы игнорируются, тк при запросе фонов. прогр. на  установку управл. терминала
	signal(SIGTTOU, SIG_IGN); // процесс(ы) получает сигналы SIGTTOU/SIGTTIN и прогр. останавливается


//Доделать Wait. обработка status
		if(pid_arr != NULL){
			for(int j=0; j < arr_progs.count; j++){
				pid = wait4(pid_arr[j], &status, 0, NULL);
				printf("pid NOT & %d\n",pid);
				if(WIFSIGNALED(status)){
					psignal(WTERMSIG(status), NULL);
				}	
			}
			free(pid_arr);
			pid_arr = NULL;
		}
		
		tcsetpgrp(STDIN_FILENO,getpid()); // возвращаем управляющий терминал

		FreeProgramForRun(&arr_progs);

		signal(SIGCHLD, handler);
		write(1,"> ",2);


	}

	if(c == -1){
		RecordError(NULL); //???????????//
	}
	
	free(string);
	//free(string2);




		
	free(arr_progs.command);
	//free(arr_progs.table.filename);
	free(arr_progs.table);
	free(arr_progs.type_cmd);

	FreePathStr(str_path);

	int cc = tcgetattr(0, &ts1);
	printf("%d\n", cc);
	//ts1.c_lflag &= ~(ECHO);
	ts1.c_lflag &= ~(ICANON | ECHO);
	int bb = tcsetattr(0, TCSANOW, &ts1);
	printf("%d\n", bb);
	
	tcsetattr(0, TCSANOW, &ts2);
	printf("{%d}\n",tcsetpgrp(STDIN_FILENO,getpid()));

	return 0;
}


