#include "stdint.h"
#include "stdlib.h"
#include "uart.h"
#include "misc.h"
#include "lcd.h"
#include "delay.h"
#include "gsm.h"
#include "system_LPC17xx.h"
#include "LPC17xx.h"
#include "stdio.h"
#include "syscalls.h"
#include "string.h"
#include "weight.h"

int32_t get_weight(void)
{
	uint32_t len = read_weighing_scale();
	int32_t valueH, valueL, value;
	if( len > 0 && strstr(uart0_fifo.line, "Kg"))
	{
		char * __next;
		valueH = strtol(uart0_fifo.line, &__next, 10);
		char * __index = index(uart0_fifo.line, '.');
		if(__index)
		{
			__index += 1;
			valueL = strtol(__index, &__next, 10);
			// Convert to grams
			value = (valueH * 1000) + valueL;
			return value;
		}
	}
	return -30000;
}