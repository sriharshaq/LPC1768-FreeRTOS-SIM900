#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char * str;
	str = (char *) malloc(512);

	if(sscanf("STATE: IP STATUS","STATE: IP %s",str))
	{
		printf("OK: %s\n",str);
	}
	else
	{
		printf("FAILED\n");
	}
	return 0;
}