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

/** @brief 		Send AT to modem and checks for OK
 *  @param 		void
 *  @return 	int8_t
 */
int8_t gsm_ping_modem(void)
{
	uint16_t len, resp;

	/* write AT to modem */
	modem_out("AT\r");

	/* read 1st line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* if it's not blank return error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	// read 2nd line it should contain 'OK'
	len = modem_readline();

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_OTHER)
	{
		if(strstr(uart3_fifo.line, "OK"))
		{
			return __LINE_PARSE_SUCCESS;
		}
		else
		{
			modem_flush_rx();
			return __MODEM_LINE_NOT_OK;
		}
	}

	modem_flush_rx();
	return __MODEM_LINE_NOT_OTHER;
}