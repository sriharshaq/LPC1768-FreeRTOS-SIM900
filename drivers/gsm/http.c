
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

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_http_head(char * host, char * path)
{
	uint8_t state, resp;
	uint16_t len;
	char buff[10];

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_send('>'))
		{

			/* request type */
			modem_out("HEAD ");
			modem_out(path);
			modem_out(" HTTP/1.1\r\n");

			/* host name */
			modem_out("Host: ");
			modem_out(host);
			modem_out("\r\n");

			/* content type */
			modem_out("Content-Type: application/json\r\n");

			/* acceptance type */
			modem_out("Accept: application/json\r\n");

			modem_out("\r\n");

			// Send completed command
			modem_putc(0x1A);

			/* read 1st line it should blank */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* if it's not blank return error */
			if(resp != __LINE_BLANK)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_BLANK;
			}

			/* read 2nd line it should contain 'SEND OK' */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* check for __OTHER if not found return error */
			if(resp != __LINE_OTHER)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_OTHER;
			}

			if(strstr(uart3_fifo.line, "SEND OK"))
			{
					modem_flush_rx();
					return __LINE_PARSE_SUCCESS;
			}
		}
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_http_get(char * host, char * path)
{
	uint8_t state, resp;
	uint16_t len;
	char buff[10];

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_send('>'))
		{

			modem_flush_rx();
			/* request type */
			// modem_out("GET ");
			// modem_out(path);
			// modem_out(" HTTP/1.1\r\n");

			// /* host name */
			// modem_out("Host: ");
			// modem_out(host);
			// modem_out("\r\n");

			// /* content type */
			// modem_out("Content-Type: application/json\r\n");

			// /* acceptance type */
			// modem_out("Accept: application/json\r\n");

			// modem_out("\r\n");

			// /* Send completed command */
			// modem_putc(0x1A);


			modem_out("GET ");
			modem_out(path);
			modem_out(" HTTP/1.1\r\n");

			// 2nd line
			modem_out("Host: ");
			modem_out(host);
			modem_out("\r\n");

			// 3rd line
			modem_out("Content-Type: application/json\r\n");

			// 4th line
			modem_out("Accept: application/json\r\n");

			modem_out("\r\n");

			modem_putc(0x1A);

			//_delay_us(500);

			/* read 1st line it should blank */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);
			//debug_out(uart3_fifo.line);

			/* if it's not blank return error */
			if(resp != __LINE_BLANK)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_BLANK;
			}

			/* read 2nd line it should contain 'SEND OK' */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);
			debug_out(uart3_fifo.line);

			/* check for __OTHER if not found return error */
			if(resp != __LINE_OTHER)
			{
				//modem_flush_rx();
				return __MODEM_LINE_NOT_OTHER;
			}

			if(strstr(uart3_fifo.line, "SEND OK"))
			{
					//modem_flush_rx();
					return __LINE_PARSE_SUCCESS;
			}
		}
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_http_put(char * host, char * path, char * dat)
{
	uint8_t state, resp;
	uint16_t len;
	char buff[10];

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_send('>'))
		{
			//modem_flush_rx();

			/* request type */
			modem_out("PUT ");
			modem_out(path);
			modem_out(" HTTP/1.1\r\n");

			/* host name */
			modem_out("Host: ");
			modem_out(host);
			modem_out("\r\n");

			/* content type */
			modem_out("Content-Type: application/json\r\n");

			/* acceptance type */
			modem_out("Accept: application/json\r\n");

			modem_out("\r\n");

			/* Content Length */
			modem_out("Content-Length: ");
			sprintf(buff, "%d", strlen(dat));
			modem_out(buff);

			modem_out("\r\n\r\n");
			modem_out(dat);
			modem_out("\r\n\r\n");

			/* Send completed command */
			modem_putc(0x1A);

			/* read 1st line it should blank */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* if it's not blank return error */
			if(resp != __LINE_BLANK)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_BLANK;
			}

			/* read 2nd line it should contain 'SEND OK' */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* check for __OTHER if not found return error */
			if(resp != __LINE_OTHER)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_OTHER;
			}

			if(strstr(uart3_fifo.line, "SEND OK"))
			{
					//modem_flush_rx();
					return __LINE_PARSE_SUCCESS;
			}
		}
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_http_post(char * host, char * path, char * dat)
{
	uint8_t state, resp;
	uint16_t len;
	char buff[10];

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_send('>'))
		{

			/* request type */
			modem_out("POST ");
			modem_out(path);
			modem_out(" HTTP/1.1\r\n");

			/* host name */
			modem_out("Host: ");
			modem_out(host);
			modem_out("\r\n");

			/* content type */
			modem_out("Content-Type: application/json\r\n");

			/* acceptance type */
			modem_out("Accept: application/json\r\n");

			modem_out("\r\n");

			/* Content Length */
			modem_out("Content-Length: ");
			sprintf(buff, "%d", strlen(dat));
			modem_out(buff);

			modem_out("\r\n\r\n");
			modem_out(dat);
			modem_out("\r\n\r\n");

			/* Send completed command */
			modem_putc(0x1A);

			/* read 1st line it should blank */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* if it's not blank return error */
			if(resp != __LINE_BLANK)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_BLANK;
			}

			/* read 2nd line it should contain 'SEND OK' */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* check for __OTHER if not found return error */
			if(resp != __LINE_OTHER)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_OTHER;
			}

			if(strstr(uart3_fifo.line, "SEND OK"))
			{
					//modem_flush_rx();
					return __LINE_PARSE_SUCCESS;
			}
		}
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_http_delete(char * host, char * path)
{
	uint8_t state, resp;
	uint16_t len;

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_send('>'))
		{

			/* request type */
			modem_out("DELETE ");
			modem_out(path);
			modem_out(" HTTP/1.1\r\n");

			/* host name */
			modem_out("Host: ");
			modem_out(host);
			modem_out("\r\n");

			/* content type */
			modem_out("Content-Type: application/json\r\n");

			/* acceptance type */
			modem_out("Accept: application/json\r\n");

			modem_out("\r\n");

			/* Send completed command */
			modem_putc(0x1A);

			/* read 1st line it should blank */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* if it's not blank return error */
			if(resp != __LINE_BLANK)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_BLANK;
			}

			/* read 2nd line it should contain 'SEND OK' */
			len 	= modem_readline();
			resp 	= process_response(uart3_fifo.line,len);

			/* check for __OTHER if not found return error */
			if(resp != __LINE_OTHER)
			{
				modem_flush_rx();
				return __MODEM_LINE_NOT_OTHER;
			}

			if(strstr(uart3_fifo.line, "SEND OK"))
			{
					//modem_flush_rx();
					return __LINE_PARSE_SUCCESS;
			}
		}
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}

void http_read_data(void)
{
	uint16_t len;


	len = modem_readline();
	debug_out(uart3_fifo.line);

	do
	{
		len = modem_readline();
		debug_out(uart3_fifo.line);
	}
	while(isblankstr(uart3_fifo.line, len) != 1);

	uint16_t _i = 0;
	while(uart3_fifo.num_bytes > 0)
	{
		char c = uart3_getc();
		//debug_putc(c);
		modem.httpdata[_i++] = c;
		vTaskDelay(2);
	}
	modem.httpdata[_i] = '\0';
}