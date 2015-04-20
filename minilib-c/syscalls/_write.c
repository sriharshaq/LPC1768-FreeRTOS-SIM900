
#include <stddef.h>
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "uart.h"
#include "syscalls.h"

int _write(int file, char * ptr, int len)
{
	return -1;	
}