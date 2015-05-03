
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

/** @brief 		Send AT+CSTT? to modem and gets the accesspoint name (After success it fills name to struct->getapn)
 *  @param 		Modem_Type_t
 *  @return 	int8_t
 */
int8_t gsm_get_network_reg_state(void)
{
	uint16_t len, resp;

	/* send AT+CSTT? to modem */
	modem_out("AT+CREG?\r");

	/* read 1st line and it should be blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for blank if not returns error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	/* read 2nd line it should have __DATA */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	if(resp != __LINE_DATA)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_DATA;
	}
	
	/*  search for characters to parse apn name 
		example line: <CR><LF>+CREG: 0,1
	*/
	char * __comma = memchr(uart3_fifo.line, ',', len) + 1;

	if(__comma == NULL)
	{
		modem_flush_rx();
		return __MODEM_LINE_CS_ERROR;
	}

	char * next;
	modem.nw_reg = strtol(__comma, &next, 10);

	/* read 3rd line it should be blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for blank if not returns error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	/* read 4th line it should contain 'OK' */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __OTHER if not found return error */
	if(resp != __LINE_OTHER)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	/* search for 'OK' */
	if(strstr(uart3_fifo.line, "OK"))
	{
		modem_flush_rx();
		return __LINE_PARSE_SUCCESS;
	}

	/* otherwise returns error */
	modem_flush_rx();
	return __MODEM_LINE_NOT_OK;
}