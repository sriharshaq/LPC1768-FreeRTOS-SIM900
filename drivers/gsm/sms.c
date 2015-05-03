
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

int8_t gsm_set_text_mode(uint8_t _mode)
{
	uint16_t len, resp;

	/* write AT to modem */
	modem_out("AT+CMGF=");
	modem_putc(_mode + 48);
	modem_putc('\r');

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

int8_t gsm_send_sms(char * num, char * msg)
{
	uint16_t len, resp;

	if(gsm_set_text_mode(1))
	{
		/* write AT to modem */
		modem_out("AT+CMGS=\"");
		modem_out(num);
		modem_out("\"\r");

		if(gsm_send('>'))
		{
			modem_out(msg);
			modem_putc(0x1A);	

			/* read 1st line it should be blank */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* check for blank if not return error */
			if(resp != __LINE_BLANK)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_BLANK;
			}

			/* read 2nd line it should be contain data */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* check whether response is data? */
			if(resp != __LINE_DATA)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_DATA;
			}

			char * __next;
			char * __index = index(uart3_fifo.line, ':');

			/* check for result and returns error */
			if(__index == NULL)
			{
				modem_flush_rx();
				return __MODEM_LINE_CS_ERROR;
			}

			/* +2 for removing :<space> */
			__index += 2;

			int8_t loc = strtol(__index, &__next, 10);

			modem.lastsmsloc = loc;

			/* read 3rd line it should be blank */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			// check whether blank line
			if(resp != __LINE_BLANK)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_BLANK;
			}


			/* read 3rd line it should be 'OK' */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* check for __OTHER */
			if(resp != __LINE_OTHER)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_OTHER;
			}

			/* check for 'OK' */
			if(strstr(uart3_fifo.line, "OK"))
			{
				modem_flush_rx();
				return __LINE_PARSE_SUCCESS;
			}

			modem_flush_rx();
			return __MODEM_LINE_NOT_OK;
		}
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}