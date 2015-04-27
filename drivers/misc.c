
#include "stdint.h"
#include "string.h"

/* check string whether blank or not */
uint8_t isblankstr(char * str, uint16_t len)
{
	uint16_t i;
	uint16_t _count = 0;
	for(i = 0;i < len;i++)
	{
		// space, tab, cr, lf
		if(str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n')
			_count++;
	}

	if(_count == len)
		return 1;			// blank line
	return 0;				// not blank
}

uint8_t strtolower(char * str)
{
	uint8_t i;
	for(i = 0;str[i] != '\0';i++)
	{
		char c = str[i];
		if(isalpha(c))
			str[i] = tolower(c);
	}
	return i;
}