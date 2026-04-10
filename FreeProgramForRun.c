#include <unistd.h>
#include <stdlib.h>
#include "StructProgForRun.h"

#include "FreeProgramForRun.h"
#include "ConstForTypeCmd.h"

void FreeProgramForRun(struct ProgForRun *arr_progs)
{
	struct Cmd *tmp5;
	for(int i=0;i<arr_progs->count;i++){
		int j = i;
		while(arr_progs->command[j]){
			tmp5 = arr_progs->command[j];
			arr_progs->command[j] = arr_progs->command[j]->next;
			free(tmp5->word);
			free(tmp5);
		}
		arr_progs->command[i] = NULL;
	
		if(arr_progs->table[i] != NULL){
			free(arr_progs->table[i]->fd_streams);
			if(arr_progs->table[i]->filename[0]){
				free(arr_progs->table[i]->filename[0]);
				arr_progs->table[i]->filename[0] = NULL;
			}
			if(arr_progs->table[i]->filename[1]){
				free(arr_progs->table[i]->filename[1]);
				arr_progs->table[i]->filename[1] = NULL;
			}
			if(arr_progs->table[i]->filename[2]){
				free(arr_progs->table[i]->filename[2]);
				arr_progs->table[i]->filename[2] = NULL;
			}
			free(arr_progs->table[i]->filename);
			free(arr_progs->table[i]);
			
		}
		arr_progs->type_cmd[i] = STANDART_CMD;
	}

	arr_progs->count = 0;
	//arr_progs.size = 5; // размер менять не буду

}

