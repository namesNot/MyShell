#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "RecordError.h"

void RecordError(const char *str)
{
	if(str){
		write(1,str,strlen(str));
		return ;
	}
	//int err = errno;
	write(4,strerror(errno),strlen(strerror(errno)));
	write(1,strerror(errno),strlen(strerror(errno)));
	exit(5);
	
	//if(res == -1){
	//	if(errno == ENOENT){
	//		//записывать в системный журнал ошибку err и ошибку write
	//	}
	//}
}


void IfErrorMalloc(void *first)
{
	if(first == NULL){
		RecordError("Не удалось выделить динамическую память\n");
		exit(1);
	}
}

