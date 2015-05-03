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


/** @brief 		Send AT+CIPS to modem and stores the result in given struct
 *  @param 		int8_t
 *  @return 	int8_t
 */
int8_t gsm_get_tcpstatus(void)
{
	uint16_t len, resp;

	/* write AT+CIPSTATUS to modem */
	modem_out("AT+CIPSTATUS\r");

	/* read 1st line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* if it's not blank return error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	/* read 2nd line it should contain 'OK' */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __LINE_OTHER */
	if(resp == __LINE_OTHER)
	{
		/* Search for OK */
		if(strstr(uart3_fifo.line, "OK") == NULL)
		{
			modem_flush_rx();
			return __MODEM_LINE_NOT_OK;
		}
	}

	/* If it's not __LINE_OTHER */
	else
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	
	/* read 3rd line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for blank line */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}


	/* read 1st line it should contain TCPSTATUS */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for  __LINE_OTHER*/
	if(resp != __LINE_OTHER)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	/* Search for char : */
	char * __index = memchr(uart3_fifo.line, ':', len);

	/* Check result */
	if(__index == NULL)
	{
		modem_flush_rx();
		return __MODEM_LINE_CS_ERROR;
	}

	/* +2 for removing :<space> */
	__index += 2;

	/* find the length of string */
	uint8_t __indexlen 	= strlen(__index);

	/* copy string to struct */
	memcpy(modem.tcpstatus, __index, __indexlen);

	/* return SUCCESS */
	return __LINE_PARSE_SUCCESS;
}

/** @brief 		Send AT+CIPCLOSE to modem and checks for 'OK'
 *  @param 		void
 *  @return 	int8_t
 */
int8_t gsm_tcp_disconnect(void)
{
	uint16_t len, resp;

	/* send AT+CIPCLOSE to modem */
	modem_out("AT+CIPCLOSE\r");

	/* read 1st line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* if it's not blank return error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	/* read 2nd line it should contain __DATA */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __OTHER if not found return error */
	if(resp != __LINE_OTHER)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	if(strstr(uart3_fifo.line, "CLOSE OK"))
	{
		modem_flush_rx();
		return __LINE_PARSE_SUCCESS;	
	}

	modem_flush_rx();
	return __MODEM_LINE_PARSE_ERROR;
}

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_tcp_connect(char * host, char * port)
{
	uint16_t len, resp;

	// send 'AT'
	modem_out("AT+CIPSTART=\"TCP\",\"");
	modem_out(host);
	modem_out("\",");
	modem_out(port);
	modem_out("\r");

	/* read 1st line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* if it's not blank return error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	/* read 2nd line it should contain __OTHER */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __OTHER if not found return error */
	if(resp != __LINE_OTHER)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	/* search for 'OK' */
	if(strstr(uart3_fifo.line, "OK") == NULL)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OK;
	}

	/* read 1st line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* if it's not blank return error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}	

	/* read 2nd line it should contain __OTHER */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __OTHER if not found return error */
	if(resp != __LINE_OTHER)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	if(strstr(uart3_fifo.line, "CONNECT OK"))
	{
		modem_flush_rx();
		return __LINE_PARSE_SUCCESS;
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}

/** @brief 		Send AT+CIPCLOSE to modem and checks for 'OK'
 *  @param 		void
 *  @return 	int8_t
 */
int8_t gsm_tcp_shutdown(void)
{
	uint16_t len, resp;

	/* send AT+CIPCLOSE to modem */
	modem_out("AT+CIPSHUT\r");

	/* read 1st line it should blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* if it's not blank return error */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	/* read 2nd line it should contain __DATA */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for __OTHER if not found return error */
	if(resp != __LINE_OTHER)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_OTHER;
	}

	if(strstr(uart3_fifo.line, "SHUT OK"))
	{
		modem_flush_rx();
		return __LINE_PARSE_SUCCESS;	
	}

	modem_flush_rx();
	return __MODEM_LINE_PARSE_ERROR;
}

/** @brief 		
 *  @param 		
 *  @return 	int8_t
 */
int8_t gsm_send(char c)
{
	/* send AT+CIPSEND to modem */
	modem_out("AT+CIPSEND\r");

	// CAUTION: Blocking
	while(uart3_fifo.num_bytes == 0);

	if(modem_getc() == c)
		return 1;

	return -1;	
}