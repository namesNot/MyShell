#include "CheckSeparators.h"

int CheckSeparators(char symb)
{
	switch(symb){
	case '<':
	      return '<';
	case '>':
	      return '>';
	case '&':
	      return '&';
	case '|':
	      return '|';
	}

	return 0;

}

