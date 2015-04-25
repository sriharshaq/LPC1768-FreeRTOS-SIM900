

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
	modem.ipstate 	= (char *) malloc(32);
	modem.apnget 	= (char *) malloc(64);
	modem.apnset 	= (char *) malloc(64);
	modem.opr 		= (char *) malloc(64);
	modem.ip 		= (char *) malloc(32);
	modem.httpdata 	= (char *) malloc(1024);
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

/** @brief 		Send AT+CIPS to modem and stores the result in given struct
 *  @param 		int8_t
 *  @return 	int8_t
 */
int8_t gsm_get_tcpstatus(Modem_Type_t * t)
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
	memcpy(t->tcpstatus, __index, __indexlen);

	/* return SUCCESS */
	return __LINE_PARSE_SUCCESS;
}

/** @brief 		Send AT+CSQ to modem and get the signal strength stores result in struct
 *  @param 		Modem_Type_t
 *  @return 	int8_t
 */
int8_t gsm_get_rssi(Modem_Type_t * t)
{
	uint16_t len, resp;

	/* write AT+CIPSTATUS to modem */
	modem_out("AT+CSQ\r");

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

	int8_t rssi = strtol(__index, &__next, 10);

	if(rssi == -1)
	{
		modem_flush_rx();
		return __MODEM_LINE_PARSE_ERROR;
	}

	t->rssi = rssi;

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

/** @brief 		Send AT+CSTT=\"APN\" to modem and check for response 'OK'
 *  @param 		Modem_Type_t (setapn should be set to accesspoint name before calling this function)
 *  @return 	int8_t
 */
int8_t gsm_set_accesspoint(Modem_Type_t * t)
{
	uint16_t len, resp;

	/* check whether struct contain valid apn */
	if(strlen(t->setapn) == 0)
		return __PARAM_PASS_VALUE_ERROR;

	/* register access point */
	modem_out("AT+CSTT=\"");
	modem_out(t->setapn);
	modem_out("\"\r");

	/* read 1st line and it should be blank */
	len 	= modem_readline();
	resp 	= process_response(uart3_fifo.line,len);

	/* check for blank */
	if(resp != __LINE_BLANK)
	{
		modem_flush_rx();
		return __MODEM_LINE_NOT_BLANK;
	}

	
	/* read 2nd line it should contain 'OK' */
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

/** @brief 		Send AT+CSTT? to modem and gets the accesspoint name (After success it fills name to struct->getapn)
 *  @param 		Modem_Type_t
 *  @return 	int8_t
 */
int8_t gsm_get_accesspoint(Modem_Type_t * t)
{
	uint16_t len, resp;

	/* send AT+CSTT? to modem */
	modem_out("AT+CSTT?\r");

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
		example line: <CR><LF>+CSTT: "TATA.DOCOMO.INTERNET","",""<CR><LF>
	*/
	char * __index = memchr(uart3_fifo.line, ':', len);
	char * __comma = memchr(uart3_fifo.line, ',', len);

	if(__index == NULL || __comma == NULL)
	{
		modem_flush_rx();
		return __MODEM_LINE_CS_ERROR;
	}

	// to remove :<space>'"' "
	__index += 3;

	// To remove '"' 
	__comma -= 1;

	uint8_t _index_len = strlen(__index);
	uint8_t _comma_len = strlen(__comma);

	memcpy(t->getapn, __index , _index_len - _comma_len);
		
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

/** @brief 		Send AT+COPS? to modem and stores operator name in struct->operator_name
 *  @param 		Modem_Type_t
 *  @return 	int8_t
 */
int8_t gsm_get_operator_name(Modem_Type_t * t)
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
	memcpy(modem->operator_name, __index , _index_len - 3);


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
int8_t gsm_get_ipaddress(Modem_Type_t * t)
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
			strcpy(t->ip_addr, uart3_fifo.line);
			modem_flush_rx();
			return __LINE_PARSE_SUCCESS;
		}
	}
	return __MODEM_LINE_PARSE_ERROR;
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

// TODO: Connection timeout has to set
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

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_tcp_send(char c)
{
	/* send AT+CIPSEND to modem */
	modem_out("AT+CIPSEND\r");

	// CAUTION: Blocking
	while(uart3_fifo.num_bytes == 0);

	if(modem_getc() == c)
		return 1;

	return -1;	
}

/** @brief 		Send AT+CIPSTART to modem and checks for 'CONNECT OK'
 *  @param 		host, port (strings)
 *  @return 	int8_t
 */
int8_t gsm_http_head(char * host, char * path)
{
	uint8_t state, resp;
	uint16_t len;

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_tcp_send('>'))
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

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_tcp_send('>'))
		{

			/* request type */
			modem_out("GET ");
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
int8_t gsm_http_put(char * host, char * path, char * dat)
{
	uint8_t state, resp;
	uint16_t len;

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_tcp_send('>'))
		{

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
int8_t gsm_http_post(char * host, char * path, char * dat)
{
	uint8_t state, resp;
	uint16_t len;

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_tcp_send('>'))
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
int8_t gsm_http_delete(char * host, char * path)
{
	uint8_t state, resp;
	uint16_t len;

	if(gsm_tcp_connect(host, "80"))
	{
		if(gsm_tcp_send('>'))
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
					modem_flush_rx();
					return __LINE_PARSE_SUCCESS;
			}
		}
	}
	modem_flush_rx();
	return __MODEM_UNKNOWN_ERROR;
}