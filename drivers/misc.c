
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
	else
		return 0;			// not blank
}

