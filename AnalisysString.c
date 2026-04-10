#include <stdlib.h>

#include "RecordError.h"
#include "StructProgForRun.h"
#include "CheckSeparators.h"
#include "ReturnValues.h"
#include "FreeProgramForRun.h"
#include "AnalisysString.h"
#include "ConstForTypeCmd.h"



#define NO_SPACE_AND_TAB_AND_ENDSTR str[pos_in_str] != ' ' && str[pos_in_str] != 9 && str[pos_in_str] != '\0'
#define SPACE_OR_TAB_OR_ENDSTR str[pos_in_str] == ' ' || str[pos_in_str] == '\0' || str[pos_in_str] == 9


const char *error_redirect_stream = "Error redirecting the input/output stream\n";
const char *unexpected_error_nextto_characters = "Unexpected error next to control characters\n";
const char *error_background_mode = "Could not set the background mode\n";
const char *no_file = "No file for redirect stream\n";

struct VariablesForAnalisysString{
	int pos_new_word, qout_marks, next_willbe_simple_symb, need_file, conveyer, change_stream, add_in_file;
	struct Cmd *first, *last;
	char redirect_symb;
};


struct Cmd* FreeMemory(struct Cmd *first)
{
	struct Cmd *tmp;
	while(first){
		tmp = first;
		free(first->word);
		first = first->next;
		free(tmp);
	}

	return NULL;
}




static void AddElement(struct Cmd **f, struct Cmd **l, int len_new_word )
{
	struct Cmd *first, *last;
	first = *f;
	last = *l;

	if(first == NULL){
		first = malloc(sizeof(struct Cmd));
		IfErrorMalloc(first);
		last = first;
		*f = first;	
	}
	else{
		last->next = malloc(sizeof(struct Cmd));
		IfErrorMalloc(last->next);
		last = last->next;
	}
	last->next = NULL;
							     //фактический размер будет меньше, если будут ковычки,которые не	
	last->word = malloc(sizeof(char) * (len_new_word)+1);//буду записаны в слово.
							     //Данная особенность отработана не будет, ради упрощения

	IfErrorMalloc(last->word);
	*l = last;

}

static void WriteWordInElem(char *str, int qout_marks, int pos_new_word, int len_word, struct Cmd *last)
{
	int need_record_next_symb = 0;
	int j;
	for(j=0; pos_new_word < len_word; pos_new_word++,j++){
		if(str[pos_new_word] == '\\' && need_record_next_symb != 1){ //второе условие в if надо
							//для экранирования и записи в строку символа '\'
			need_record_next_symb = 1;
			j--;
			continue;
		}
		if(qout_marks > 0 && str[pos_new_word] == '\"' && need_record_next_symb != 1){ //нужно для пропуска "
			j--;
			continue;
		}
		need_record_next_symb = 0; // необходим тк после первого if переходим на новую итерацию
					   // выполняется второй if. и чтобы снять флаг на 
					   // need_record-next-symb выполняем эту строку
		last->word[j] = str[pos_new_word];
	}
	last->word[j] = '\0';
}

static void GetFilename(struct TableRedirect *table, char *str, int pos_filename, int cur_pos, int numb_stream)
{
	int i, USED = 1;

	table->fd_streams[numb_stream] = USED;
	table->filename[numb_stream] = malloc(sizeof(char)*(cur_pos-pos_filename)+1);
	IfErrorMalloc(table->filename[numb_stream]);

	for(i=0; pos_filename < cur_pos; i++, pos_filename++){
		if(str[pos_filename] == '\"'){
			i--;
			continue;
		}
			
		((table->filename)[numb_stream])[i] = str[pos_filename];
	}
	table->filename[numb_stream][i] = '\0';
}



static char ProcessingRedirectSymb(char *str, int cur_pos, struct TableRedirect *table, struct VariablesForAnalisysString *var)
{
	int USED = 1;
	switch(var->redirect_symb){
		case '<':
			if(table->fd_streams[0] == USED)
				return 0;
			GetFilename(table, str, var->pos_new_word, cur_pos, 0);
			break;
		case '>':
			if(table->fd_streams[1] == USED)
				return 0;
			GetFilename(table, str, var->pos_new_word, cur_pos, 1);
			break;
	}

	return 1;
}


static struct Cmd *AddBackgroundMode(struct Cmd * command, struct Cmd *add)
{
	struct Cmd *tmp = command;    // Ищет последний элемент и добавляет в конец &
	for(;tmp->next;tmp=tmp->next) //
	{	}		      //
	
	tmp->next = add;

	return NULL;
}

static void *CreateCommand(struct ProgForRun *arr_progs,char conveyer, int mode, struct Cmd *first)
{
	arr_progs->command[arr_progs->count] = first;
	arr_progs->table[arr_progs->count] = NULL;
	if(conveyer){
		arr_progs->type_cmd[arr_progs->count] = PART_OF_CONVEYOR; // 2
	}
	else{
		arr_progs->type_cmd[arr_progs->count] = STANDART_CMD; // 2
	}
	
	arr_progs->count++;

	return NULL;
}

static void CreateTable(struct TableRedirect **table, int count, int add)
{
	int NO_USED = 0;
	table[count] = malloc(sizeof(struct TableRedirect )*1);
	IfErrorMalloc(table[count]);
	
	table[count]->fd_streams = malloc(sizeof(int) * 3);
	IfErrorMalloc(table[count]->fd_streams);
	table[count]->fd_streams[0] = NO_USED;
	table[count]->fd_streams[1] = NO_USED;
	table[count]->fd_streams[2] = NO_USED;

	table[count]->filename = malloc(sizeof(char **) * 3);
	IfErrorMalloc(table[count]->filename);
	table[count]->filename[0] = NULL;
	table[count]->filename[1] = NULL;
	table[count]->filename[2] = NULL;

	table[count]->add_in_file = add;
}

static int IfNeedFile(struct ProgForRun *arr_progs, char *str, int pos_in_str, struct VariablesForAnalisysString *var )
{
	int result = 0, SET = 1;
	if(var->change_stream != SET && arr_progs->count != 0){
		CreateTable(arr_progs->table, arr_progs->count-1, var->add_in_file);
		var->add_in_file = 0;
	}
	result = ProcessingRedirectSymb(str, pos_in_str, arr_progs->table[arr_progs->count-1], var);
	if(result == 0){
		RecordError(error_redirect_stream);
		return 0;
	}

	return 1;
}

static int BackgroundMode(char *str, int pos_in_str,struct ProgForRun *arr_progs, struct VariablesForAnalisysString *var)
{
	int NO_SET = -1;
	
	if(var->pos_new_word == NO_SET && var->first == NULL && arr_progs->count == 0){	 // Случай когда: & word
		return 0;									
	}												
														
	if(var->need_file || var->change_stream){					// Случай когда: .. < word& 
		if(IfNeedFile(arr_progs, str, pos_in_str, var ) == 0){		// или < word > word& .Т.е. & после перенаправ.
			return 0; 						// потока. Здесь получается тоже pos_new_file ==
		}								// NO_SET, но вызываем IfNeedFile потому что
		var->change_stream = var->need_file = var->redirect_symb = 0;	//обрабатывается у нас имя файла по другому
		var->pos_new_word = NO_SET;					//
	}
	else if(var->pos_new_word != NO_SET){							// А здесь для других случаях
		AddElement(&var->first, &var->last, pos_in_str - var->pos_new_word);		// когда & сразу после word: word&
		WriteWordInElem(str, var->qout_marks, var->pos_new_word, pos_in_str, var->last); // Создаем этот word
	}

	if(var->conveyer){							//Добавлям как команду готовую
		var->first = CreateCommand(arr_progs, var->conveyer, 2, var->first);	//
		var->conveyer = 0;						//
	}
	else if(var->first){
		var->first = CreateCommand(arr_progs, var->conveyer, 0, var->first); //  Если простое word &, без каналов 
										     //  и перенаправ.
	}

	var->pos_new_word = pos_in_str;							  //Создается слово для &
	AddElement(&var->first, &var->last, 1);						  //
	WriteWordInElem(str, var->qout_marks, var->pos_new_word, pos_in_str+1, var->last);//	

	var->first = AddBackgroundMode(arr_progs->command[arr_progs->count-1], var->first); // И добавляется в команду
												
	var->pos_new_word = NO_SET;
	return CONTINUE;
}


/*void AddMemoryConveyer(struct Conveyer **conv)
{
	(*conv)->size_item = (*conv)->size_item + 5;
	int **tmp = malloc(sizeof(int **) * (*conv)->size_item);
	for(int i=0; i < ((*conv)->size_item - 5); i++){
		tmp[i] = (*conv)->pipes[i];
	}
	free((*conv)->pipes);
	(*conv)->pipes = tmp; 
	
	struct Cmd **tmp2 = malloc(sizeof(struct Cmd **) * (*conv)->size_item);
	for(int i=0; i < ((*conv)->size_item - 5); i++){
		tmp2[i] = (*conv)->command[i];
	}
	free((*conv)->command);
	(*conv)->command = tmp2; 
}
*/






static int PipelineProcessing(int pos_in_str, char *str, struct ProgForRun **arr_progs, struct VariablesForAnalisysString *var)
{
	int NO_SET = -1;

	if(var->need_file || var->change_stream){					//Выполняется после перенаправл: ..<file| 
		if(IfNeedFile(*arr_progs, str, pos_in_str, var ) == 0){		// 
			return 0; 
		}
		var->redirect_symb = var->need_file = var->pos_new_word = 0;
	}
	else if(var->pos_new_word != NO_SET){							 
		AddElement(&var->first, &var->last, pos_in_str - var->pos_new_word);		///создаю first если word|, т.е.
		WriteWordInElem(str, var->qout_marks, var->pos_new_word, pos_in_str, var->last);// | сразу после word
	}
												
	if(var->first){ 
		var->first = CreateCommand(*arr_progs, var->conveyer, PART_OF_CONVEYOR, var->first); 	//если first то 
													//создаю команду
	}
	else if((*arr_progs)->count == 0){	// Это случай когда | word, т.е. до | нет слов
		RecordError(unexpected_error_nextto_characters);
		return 0;
	}
	var->conveyer = 1;		
	var->pos_new_word = NO_SET;

	return 1;
}

static int GettingFile(char *str, int pos_in_str, struct ProgForRun *arr_progs, struct VariablesForAnalisysString *var )
{
	int NO_SET = -1;
	if(IfNeedFile(arr_progs, str, pos_in_str, var ) == 0){
		FreeProgramForRun(arr_progs);	
		var->first = FreeMemory(var->first);
		return 0;	
	}
	var->redirect_symb = 0;
	var->pos_new_word = NO_SET;

	var->redirect_symb = CheckSeparators(str[pos_in_str]);
	var->change_stream = 1;     	//change_stream обрабатывается в другом месте
	return 1;


}


static int SeparatorsProcessing(int *pos_in_str, char *str, struct ProgForRun *arr_progs, struct VariablesForAnalisysString *var)
{
	int NO_SET = -1;

	if(var->first == NULL && var->pos_new_word == NO_SET && arr_progs->count == 0){ // Случай: < file of <file, т.е.
		RecordError(unexpected_error_nextto_characters);			// до перенаправ нет ничего. count == 0 
		return 0;							// нужен чтобы отличить от <.. > file, т.е. от 
										// еще одного перенаправления
	}
	if(var->need_file){						// выполняется когда получаем < or > , т.е. повторное 
		if(GettingFile(str, *pos_in_str, arr_progs, var) == 0){	// перенаправление: < .. > file
			return 0;					//
		}							//
	}

	var->redirect_symb = CheckSeparators(str[*pos_in_str]);         
	
	if(var->pos_new_word != NO_SET){ 							 
		AddElement(&var->first, &var->last, *pos_in_str - var->pos_new_word);		 
		WriteWordInElem(str, var->qout_marks, var->pos_new_word, *pos_in_str, var->last);
		var->pos_new_word = NO_SET;							
	}											 

	if(var->conveyer){									
		var->first = CreateCommand(arr_progs, var->conveyer, PART_OF_CONVEYOR, var->first); // Если |word < или | word <
		//if(conv->count == conv->size_item){						    // т.е. слово идет после |
		//	AddMemoryConveyer(&conv);
		//}
		//arr_progs->count--;
		var->conveyer = 0;
	}
	if(var->first){								
		var->first = CreateCommand(arr_progs, var->conveyer, STANDART_CMD, var->first);	
	}									

	if(arr_progs->count > 0 && arr_progs->table[arr_progs->count-1] != NULL){ // Если word имеет table, значит было перенаправ
										  // вв/вы, а значит это не первое перенаправл
		var->change_stream = 1;//change_stream обрабатывается в другом месте
	}
	else
		var->need_file = 1; 						  // Первое перенаправление

	return 1;
}



static int CreateNewWord(struct ProgForRun *arr_progs, char *str, int pos_in_str, struct VariablesForAnalisysString *var)
{
	int NO_SET = -1;
	if(var->need_file || var->change_stream){
		if(IfNeedFile(arr_progs, str, pos_in_str, var) == 0){
			return 0;	
		}
		var->change_stream = var->need_file = var->redirect_symb = 0;
		var->pos_new_word = NO_SET;
		return 1;
	}


	AddElement(&var->first, &var->last, pos_in_str - var->pos_new_word);
	WriteWordInElem(str, var->qout_marks, var->pos_new_word, pos_in_str, var->last);
	var->pos_new_word = NO_SET;

						
	if(var->conveyer && str[pos_in_str] == '\0'){
		/*if(conv->count == conv->size_item){
			AddMemoryConveyer(&conv);
		}*/
		var->first = CreateCommand(arr_progs, var->conveyer, PART_OF_CONVEYOR, var->first);
		var->conveyer = 0;
		return 1;

	}
		
	if(str[pos_in_str] == '\0'){
		var->first = CreateCommand(arr_progs, var->conveyer, STANDART_CMD, var->first);
		return 1;
	}

	return 1;
}



static int SymbolProcessing(char *str, int pos_in_str,struct ProgForRun *arr_progs, struct VariablesForAnalisysString *var)
{
	int NO_SET = -1;

	if(str[pos_in_str] == '&'){
		if(BackgroundMode(str, pos_in_str, arr_progs, var) == 0){
			RecordError(error_background_mode);
			var->first = FreeMemory(var->first);
			FreeProgramForRun(arr_progs);	
			return EXIT;
		}
		return CONTINUE;
	}
	
	if((var->conveyer || var->redirect_symb) && (CheckSeparators(str[pos_in_str])) 
			&& var->pos_new_word == NO_SET && !var->first){
		if(var->redirect_symb == '>' && (CheckSeparators(str[pos_in_str])) == '>'){
			var->add_in_file = 1;	
			return CONTINUE;
		}
		RecordError(unexpected_error_nextto_characters);//Это условие действует когда word | < ,т.е когда 
								//перенаправление (<,>) 
		var->first = FreeMemory(var->first);	// следует сразу за |. А между ними должно быть слово.
		FreeProgramForRun(arr_progs);			// pos_new_word == -1 нужно чтобы срабатывало только когда там нет символа
		return EXIT;				// !first - нужно чтобы срабат только когда там нет слова
	}


	
	if(str[pos_in_str] == '|'){
		if(PipelineProcessing(pos_in_str, str, &arr_progs, var) == 0){ // создание части конвейера
			var->first = FreeMemory(var->first);
			FreeProgramForRun(arr_progs);	
			return EXIT;
		}
		return CONTINUE;
	}

	if((CheckSeparators(str[pos_in_str])) ){ 
		if(SeparatorsProcessing(&pos_in_str, str, arr_progs, var) == 0){
			var->first = FreeMemory(var->first);
			FreeProgramForRun(arr_progs);	
			return EXIT;
		}

		return CONTINUE;

	}
	if((SPACE_OR_TAB_OR_ENDSTR) && var->pos_new_word != NO_SET){
		if(CreateNewWord(arr_progs, str, pos_in_str, var) == 0){
			var->first = FreeMemory(var->first);
			FreeProgramForRun(arr_progs);
			return EXIT;
		}
	}

	return 0;
}

static void FinishTheCommand(struct VariablesForAnalisysString *var, struct ProgForRun *arr_progs)
{
	if(var->need_file || var->conveyer){
		RecordError(no_file);
		var->first = FreeMemory(var->first);
		FreeProgramForRun(arr_progs);
		return ;
	}
	
	if(var->first){
		var->first = CreateCommand(arr_progs, var->conveyer, STANDART_CMD, var->first);
	}
}

static int IsDoubleQuotes(char *str, int pos_in_str, struct VariablesForAnalisysString *var)
{
	if(str[pos_in_str] == '\\' && !var->next_willbe_simple_symb){
		var->next_willbe_simple_symb = 1;
		return CONTINUE;
	}
	if(str[pos_in_str] == '\"' && !var->next_willbe_simple_symb){
		var->qout_marks++ ;
		//continue; //пустое слово получается изза того что не используется continue.
		//дальше он выполняет pos_new_word=1,след итерацией создаетcя слово, где '\0'
		//находится в первом символе.
	}
	
	var->next_willbe_simple_symb = 0; // необходим тк после первого if переходим на новую итерацию
					   // НЕ выполняется второй if. и чтобы снять флаг на 
					   // next_willbe_simple_symb выполняем эу строку
	return 0; 
}


void AnalisysString(char *str, int len,struct ProgForRun *arr_progs)
{
	struct VariablesForAnalisysString var = {-1,0,0,0,0,0,0,NULL,NULL,0};
	const int NO_SET = -1;
	int res;


	for(int pos_in_str=0; pos_in_str < len; pos_in_str++){
		if(IsDoubleQuotes(str, pos_in_str, &var) == CONTINUE)
			continue;
		
		if(var.pos_new_word == NO_SET && NO_SPACE_AND_TAB_AND_ENDSTR && CheckSeparators(str[pos_in_str]) == 0 ){ //9 - TAB
			var.pos_new_word = pos_in_str;
		}

		if(var.qout_marks % 2 == 0){
			res = SymbolProcessing(str, pos_in_str, arr_progs, &var);
			if(res == EXIT)
				return ;
			continue; //res == CONITNUE
		}

		if(str[pos_in_str] == '\0' && var.qout_marks % 2 != 0){
			RecordError("Error: Too many symbol \'\"\'");
			var.first = FreeMemory(var.first);
			FreeProgramForRun(arr_progs);
		}
		
	}

	if(var.need_file || var.conveyer || var.first){
		FinishTheCommand(&var, arr_progs);
	}


	return ;

}


