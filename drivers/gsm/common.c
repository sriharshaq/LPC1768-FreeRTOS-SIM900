
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
#include "config.h"

Modem_Type_t modem;

/** @brief 
 *  @param 
 *  @return 
 */
void gsm_buff_init(void)
{
	/* allocate memory */
	modem.tcpstatus 	= (char *) malloc(32);
	modem.setapn 		= (char *) malloc(64);
	modem.getapn 		= (char *) malloc(64);
	modem.operator_name = (char *) malloc(64);
	modem.ip_addr 		= (char *) malloc(32);
	modem.httpheader 	= (char *) malloc(512);
	modem.httpdata 		= (char *) malloc(1024);
}

int8_t process_response(char * ptr, uint16_t len)
{
	if(isblankstr(uart3_fifo.line, len))
	{
		return __LINE_BLANK;
	}

	// Check for data
	else if(memchr(ptr, '+', len) && memchr(ptr, ':', len))
	{
		if(strstr(uart3_fifo.line, "+CPIN") || strstr(uart3_fifo.line, "+CFUN"))
		{
			return __LINE_START_OTHER;
		}
		return __LINE_DATA;
	}

	// Check for errors
	else if(strstr(ptr,"ERROR") != NULL)
	{
		return __LINE_ERROR;
	}

	// otherwise 
	else
	{
		if(strstr(uart3_fifo.line, "RDY"))
		{
			return __LINE_START_RDY;
		}
		else if(strstr(uart3_fifo.line, "Call Ready"))
		{
			return __LINE_START_CRDY;
		}
		return __LINE_OTHER;
	}
}