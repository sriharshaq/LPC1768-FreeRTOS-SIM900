
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

/* APN/OPR List size */
#define APN_OPR_LIST_LEN 8

/* List of operators */
const char * oprList[]	=	{	
								"airtel",
								"cellone",
								"idea",
								"aircel",
								"tata docomo",
								"t24",
								"reliance",
								"vodafone"
							};

/* List of Access Points for different operator */
const char * apnList[]	=	{	
								"airtelgprs.com",
								"bsnlnet",
								"internet",
								"aircelgprs.pr",
								"TATA.DOCOMO.INTERNET",
								"TATA.DOCOMO.INTERNET",
								"rcomnet",
								"www"
							};

/** @brief 		Send AT+CSTT=\"APN\" to modem and check for response 'OK'
 *  @param 		Modem_Type_t (setapn should be set to accesspoint name before calling this function)
 *  @return 	int8_t
 */
int8_t gsm_set_accesspoint(void)
{
	uint16_t len, resp;

	/* check whether struct contain valid apn */
	if(strlen(modem.setapn) == 0)
		return __PARAM_PASS_VALUE_ERROR;

	/* register access point */
	modem_out("AT+CSTT=\"");
	modem_out(modem.setapn);
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
int8_t gsm_get_accesspoint(void)
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

	memcpy(modem.getapn, __index , _index_len - _comma_len);
		
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


int8_t detect_and_set_apn(void)
{
	uint8_t flag = 0;
	if(gsm_get_operator_name())
	{
		strtolower(modem.operator_name);
		if(gsm_get_accesspoint())
		{
			for(uint8_t i = 0;i < APN_OPR_LIST_LEN;i++)
			{
				if(strstr(modem.operator_name, oprList[i]))
				{
					strcpy(modem.setapn, apnList[i]);
					flag = 1;
					break;
				}
			}
			if(flag)
			{
				if(strstr(modem.getapn, modem.setapn))
				{
					if(gsm_get_operator_name())
					{
						return 1;
					}			
				}
				else
				{
					if(gsm_set_accesspoint())
					{
						if(gsm_get_accesspoint())
						{
							if(strstr(modem.getapn, modem.setapn))
							{
								if(gsm_get_operator_name())
								{
									return 2;
								}
							}
						}
					}
				}				
			}
		}
	}
	return 0;
}