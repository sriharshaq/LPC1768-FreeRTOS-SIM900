
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

/** @brief 		Send AT+CIICR to modem and checks for 'OK' after success modem should get ip address
 *  @param 		void
 *  @return 	int8_t
 */
int8_t gsm_start_gprs(void)
{
	uint16_t len, resp;

	/* write AT+CIICR to modem */
	modem_out("AT+CIICR\r");

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

/** @brief 		Send AT+CIFSR to modem and checks for IP address if found returns success and stores in struct->ip_addr
 *  @param 		Modem_Type_t
 *  @return 	int8_t
 */
int8_t gsm_get_ip_address(void)
{
	uint16_t len, resp;

	// send 'AT+CIFSR'
	modem_out("AT+CIFSR\r");

	/* read 1st line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* if it's not blank return error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	/* read 2nd line it should contain IP address */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __OTHER if not found return error */
	if(resp != __LINE_OTHER)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	if(strstr(uart3_fifo.line, "ERROR"))
	{
		modem_flush_rx();
		return __MODEM_UNKNOWN_ERROR;
	}
	else
	{
		if(memchr(uart3_fifo.line,'.',len))
		{
			strcpy(modem.ip_addr, uart3_fifo.line);
			modem_flush_rx();
			return __LINE_PARSE_SUCCESS;
		}
	}
	return __MODEM_LINE_PARSE_ERROR;
}