
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

/** @brief 		Send AT+COPS? to modem and stores operator name in struct->operator_name
 *  @param 		Modem_Type_t
 *  @return 	int8_t
 */
int8_t gsm_get_operator_name(void)
{
	uint16_t len, resp;

	/* send AT+COPS? to modem */
	modem_out("AT+COPS?\r");

	/* read 1st line and it should be blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for blank if not returns error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}


	/* read 2nd line it should contain __DATA */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __DATA if not found return error */
	if(resp != __LINE_DATA)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	/* search for character
	   example line: +COPS: 0,0,"T24"
	*/
	char * __index = memchr(uart3_fifo.line, '"', len);

	

	if(__index == NULL)
	{
		modem_flush_rx();
		return __MODEM_LINE_CS_ERROR;
	}

	/* to remove :<space>'"' " */
	__index += 1;

	/* find string length */
	uint8_t _index_len = strlen(__index);

	/* copy operator name to struct */
	memcpy(modem.operator_name, __index , _index_len - 3);
	
	/* read 3rd line and it should be blank */
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