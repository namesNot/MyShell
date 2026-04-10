#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "LineDiscipline.h"
#include "RecordError.h"
#include "ReturnValues.h"
#include "CheckSeparators.h"

const char *error_read_directory = "Ошибка чтения файла в каталоге\n";

struct VariablesForAutoCompletion{
	int pos_string, pos_new_word, count_words, cur_pos_arr_psbl_str;
	char ch;
	char **arr_psbl_str;
};


static void AddPossibleWord(int *cur_pos_arr_psbl_str, int *size, char ***arr_psbl_str, char *d_name)
{
	if(*cur_pos_arr_psbl_str == *size){
		*size = *size + 10;
		char **tmp_str = malloc(sizeof(char*) * *size);
		IfErrorMalloc(tmp_str);

		for(int j=0; j < (*size - 10); j++)
			tmp_str[j] = (*arr_psbl_str)[j];

		free(*arr_psbl_str);
		*arr_psbl_str = tmp_str;

	}
	(*arr_psbl_str)[*cur_pos_arr_psbl_str] = malloc(sizeof(char) * strlen(d_name) + 1);
	IfErrorMalloc( (*arr_psbl_str)[*cur_pos_arr_psbl_str] );

	strcpy( (*arr_psbl_str)[*cur_pos_arr_psbl_str], d_name);
	(*arr_psbl_str)[*cur_pos_arr_psbl_str][strlen(d_name)] = '\0';
	(*cur_pos_arr_psbl_str)++;
}

static void FindSimmilaryWord(DIR *fd, int *cur_pos_in_str,int pos_new_word, char *str, int *cur_pos_arr_psbl_str, int *size, 
						char ***arr_psbl_str)
{
	int tmp, possible_word;
	struct dirent *dir;

	while((dir=readdir(fd)) != NULL){
		if(errno != 0){
			RecordError(error_read_directory);
			continue;
		}
		tmp = pos_new_word;
		possible_word = 1;

		for(int i=0;tmp < *cur_pos_in_str;i++,tmp++){
			if(str[tmp] != dir->d_name[i]){
				possible_word = 0;
				break;
			}
		}

		if(possible_word){
			AddPossibleWord(cur_pos_arr_psbl_str, size, arr_psbl_str, dir->d_name);
		}
	}
}

static char ** SearchForSimilarCmds(int pos_new_word, char *str, char **str_path,int *cur_pos, int *cur_pos_arr_psbl_str)
{
	struct stat *st_inode = malloc(sizeof(struct stat));
	DIR *fd;
	int size = 10;
	char **arr_psbl_str = malloc(sizeof(char *) * size);
	
	*cur_pos_arr_psbl_str = 0;


	for(int i=0;str_path[i] != NULL; i++){
		if(lstat(str_path[i],st_inode) == -1){ // get info about directory
			RecordError(NULL);
			return NULL; //Мб continue
		}
		if(((st_inode->st_mode & S_IFMT)) == S_IFLNK)//check on SymbLink
			continue;

		if((fd = opendir(str_path[i])) == NULL){
			RecordError(NULL);
			return NULL;
		}	

		errno = 0;
		
		FindSimmilaryWord(fd, cur_pos, pos_new_word, str, cur_pos_arr_psbl_str, &size, &arr_psbl_str);
		
		closedir(fd);
	}

	free(st_inode);


	return arr_psbl_str;
}


static char ** AddFilename(int pos_new_word, char *str, int *cur_pos, int *cur_pos_arr_psbl_str)

{	
	char *home = malloc(sizeof(char) * 40);
	IfErrorMalloc(home);

	home = getcwd(home, 40);
	IfErrorMalloc(home);

	char **str_path_2 = malloc(sizeof(char *) * 2);
	IfErrorMalloc(home);

	str_path_2[0] = home;
	str_path_2[1] = NULL;

	char **arr_psbl_str = SearchForSimilarCmds(pos_new_word, str, str_path_2, cur_pos, cur_pos_arr_psbl_str);

	chdir(home);

	free(str_path_2);
	free(home);

	return arr_psbl_str;
}

static void FreeArrPsblStr(char ***arr_psbl_str, int *cur_pos_arr_psbl_str)
{
	for(int i=0;i < *cur_pos_arr_psbl_str;i++){
		free((*arr_psbl_str)[i]);
	}
	free(*arr_psbl_str);
	*arr_psbl_str = NULL;
	*cur_pos_arr_psbl_str = 0;
}


static void PrintPossibCmds(char **arr_psbl_str, int cur_pos_arr_psbl_str)
{
	if(write(1, "\n", 1) == -1)
		RecordError(NULL);
	for(int i=0;i<cur_pos_arr_psbl_str;i++){
		if(write(1,arr_psbl_str[i],strlen(arr_psbl_str[i])) == -1)
			RecordError(NULL);
		if(write(1,"\n",1) == -1)
			RecordError(NULL);
	}

}


static void IfMore20Cmds(int cur_pos_arr_psbl_str, char **arr_psbl_str, char *str)
{
	char *numb = malloc(sizeof(char) * 4);
	IfErrorMalloc(numb);

	int res = sprintf(numb, "%d",cur_pos_arr_psbl_str);
	if(res < 0){
		RecordError(NULL);
		exit(6);
	}

	if(write(1,"\nDisplay all ",13) == -1)
		RecordError(NULL);
	if(write(1,numb,res) == -1)
		RecordError(NULL);
	if(write(1," possibilites? (y or n)\n",24) == -1)
		RecordError(NULL);

	char ch;
	if(read(0,&ch, 1) == -1)
		RecordError(NULL);
	if(ch == 'y'){
		PrintPossibCmds(arr_psbl_str, cur_pos_arr_psbl_str);
	}
	
	free(numb);
}

static void AddInWord(char **arr_psbl_str, char *str, int *cur_pos, int pos_word)
{
	if( write(1,arr_psbl_str[0]+strlen(str+pos_word),strlen(arr_psbl_str[0]+strlen(str+pos_word))) == -1)
		RecordError(NULL);
	if( write(1," ", 1) == -1)
		RecordError(NULL);

	int i = strlen(str+pos_word);
	for(; arr_psbl_str[0][i] != '\0';i++){		
		str[i+pos_word] = arr_psbl_str[0][i];
		(*cur_pos)++;
	}
	str[i+pos_word] = ' ';
	(*cur_pos)++;
	str[i+pos_word+1] = '\0';
}

static int AddWord(char *str, char **str_path, struct VariablesForAutoCompletion *var)
{
	if(var->arr_psbl_str){
		if(var->cur_pos_arr_psbl_str > 20)
			IfMore20Cmds(var->cur_pos_arr_psbl_str, var->arr_psbl_str, str);
		else
			PrintPossibCmds(var->arr_psbl_str, var->cur_pos_arr_psbl_str);
		
		FreeArrPsblStr(&var->arr_psbl_str, &var->cur_pos_arr_psbl_str);
			
		if(write(1,"> ",2) == -1)
			RecordError(NULL);
		if(write(1,str,strlen(str)) == -1)
			RecordError(NULL);


		return 0; // не важно что вернет

	}

	if(var->count_words > 0)
		var->arr_psbl_str = AddFilename(var->pos_new_word, str, &var->pos_string, &var->cur_pos_arr_psbl_str);
	else
		var->arr_psbl_str = SearchForSimilarCmds(var->pos_new_word, str, str_path, &var->pos_string, 
												&var->cur_pos_arr_psbl_str);

	if(var->cur_pos_arr_psbl_str == 1){
		AddInWord(var->arr_psbl_str, str, &var->pos_string, var->pos_new_word);
		FreeArrPsblStr(&var->arr_psbl_str, &var->cur_pos_arr_psbl_str);
		return INSERTED;
	}
	
	return 0;
			
}



static void CountingWords(struct VariablesForAutoCompletion *var)
{
	int NO_SET = -1;
	if(var->ch != ' ' && var->ch != 9){
		if(CheckSeparators(var->ch) != 0)
			return ;
		if(var->pos_new_word == NO_SET)
			var->pos_new_word = var->pos_string;
	}
	if(var->ch == ' '){
		if(var->pos_new_word != NO_SET){
			var->count_words++;
			var->pos_new_word = -1;
		}
		if(var->arr_psbl_str){
			FreeArrPsblStr(&var->arr_psbl_str, &var->cur_pos_arr_psbl_str);
		}

	}
}





static int IfControlCharacters(char *string, struct VariablesForAutoCompletion *var)
{
	int NO_SET = -1;

	if(var->ch == 127){
		if(var->pos_string != 0){
			if(write(1,"\b \b",3) == -1)
				RecordError(NULL);
			var->pos_string--;
			string[var->pos_string] = '\0';
			if(var->pos_string == 0){ //если дошли до начала строки
				var->pos_new_word = NO_SET;
				var->count_words = 0;
			}
			if(var->arr_psbl_str)
				FreeArrPsblStr(&var->arr_psbl_str, &var->cur_pos_arr_psbl_str);
		}
		return BACKSPACE;
	}
	if(var->ch == 4){
		if(var->pos_string == 0)
			return ENDINPUT;
		return ATTEMPT_ENDINPUT;
	}

	return 0;
}

static int RecordSymb(char *string, struct VariablesForAutoCompletion *var)
{
	string[var->pos_string] = var->ch;
	var->pos_string++;
	string[var->pos_string] = '\0';
	write(1, &var->ch, 1);
	if(var->ch == '\n')
		return NEWLINE;
	return 0;
}


int AutoCompletion(char *string, char **str_path)
{
	
	struct VariablesForAutoCompletion var = {0, -1, 0, 0 ,0, NULL};
	int n, res;
	int NO_SET = -1;

	while((n = read(0,&var.ch,1)) != -1){ // если конец файла Ctrl-D и в строке есть символы то что?		
		
		if( (res = IfControlCharacters(string, &var)) ){
			if(res == ENDINPUT)
				return EXIT;
			continue;	
		}
		
		CountingWords(&var);
		
		if(var.pos_new_word != NO_SET && var.ch == 9){
			res = AddWord(string, str_path, &var);
			if(res == INSERTED){
				var.arr_psbl_str = NULL;
				var.count_words++;
				var.pos_new_word = NO_SET;
			}
			continue;
		}
		
		if(RecordSymb(string, &var) == NEWLINE){
			if(var.arr_psbl_str != NULL)
				FreeArrPsblStr(&var.arr_psbl_str, &var.cur_pos_arr_psbl_str);

			return var.pos_string;
		}

	}

	return ERROR_INPUT;
}


//CheckSeparators, RecordError, IfErrorMalloc
