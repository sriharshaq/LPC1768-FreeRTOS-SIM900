

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

void gsm_allocate_mem(void)
{
	/* allocate memory */
	modem.ipstate = (char *) malloc(32);
	modem.apn = (char *) malloc(64);
	modem.opr = (char *) malloc(64);
	modem.ip = (char *) malloc(32);
}

/*
************* AT Commands *************************************

	Types: 
	-----------------------------------------------------------
 	1. 	Test Command:
		AT+<x>=?

	2.	Read Command:
		AT+<x>?

	3.	Write Command
		AT+<x>=<...>

	4. Execution command
		AT+<x>
	-----------------------------------------------------------

	-----------------------------------------------------------
	Response
	-----------------------------------------------------------
	<CR><LF><responce><CR><LF>
	-----------------------------------------------------------

	-----------------------------------------------------------
	Example:
	-----------------------------------------------------------
		Commands		: 	AT
		Type 			: 	Read Command
		Response		: 	OK
		Raw Response	:	<CR><LF>OK<CR><LF>
	-----------------------------------------------------------
*/


uint8_t process_response(char * ptr, uint16_t len)
{
	if(isblankstr(uart3_fifo.line, len))
	{
		// this is blank line
		#if PROCESS_DEBUG_INFO_LEVEL == 1 || PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out("blank line\r\n");
		#endif
		#if PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out(ptr);
			debug_out("\r\n");
		#endif

		return __LINE_BLANK;
	}

	// Check for data
	else if(memchr(ptr, '+', len) && memchr(ptr, ':', len))
	{
		// this is data
		#if PROCESS_DEBUG_INFO_LEVEL == 1 || PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out("data line\r\n");
		#endif
		#if PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out(ptr);
			debug_out("\r\n");
		#endif

		return __LINE_DATA;
	}

	// Check for errors
	else if(strstr(ptr,"ERROR") != NULL)
	{
		// this is error
		#if PROCESS_DEBUG_INFO_LEVEL == 1 || PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out("error line\r\n");
		#endif
		#if PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out(ptr);
			debug_out("\r\n");
		#endif

		return __LINE_ERROR;
	}

	// otherwise ?
	else
	{
		// this is ok
		#if PROCESS_DEBUG_INFO_LEVEL == 1 || PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out("other line\r\n");
		#endif
		#if PROCESS_DEBUG_INFO_LEVEL == 2
			debug_out(ptr);
			debug_out("\r\n");
		#endif

		return __LINE_OTHER;
	}
}

uint8_t gsm_ping_modem(void)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT'\r\n");
	#endif

	// send 'AT'
	modem_out("AT\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for 'OK'\r\n");
		#endif
		if(strstr(uart3_fifo.line, "OK"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("found ok\r\n");
				debug_out("'AT' 'OK' success\r\n");
			#endif
			// SUCCESS
			flag = 1;
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("error expected is __OTHER, but got something else\r\n");
				sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
				debug_out(debug_buff);
			#endif
		}
	}
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("'OK' not found\r\n");
			sprintf(debug_buff, "line: %s", uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}
	// FAIL
	return flag;
}


uint8_t gsm_update_ipstatus(void)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT+CIPSTATUS'\r\n");
	#endif

	// send 'AT'
	modem_out("AT+CIPSTATUS\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT+CIPSTATUS' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();
	//debug_out(uart3_fifo.line);

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for 'OK'\r\n");
		#endif
		if(strstr(uart3_fifo.line, "OK"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("found ok\r\n");
				debug_out("'AT+CIPSTATUS' 'OK' success\r\n");
			#endif
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("'OK' not found\r\n");
				sprintf(debug_buff, "line: %s", uart3_fifo.line);
				debug_out(debug_buff);
			#endif
		}
	}

	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __OTHER, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 3rd line\r\n");
	#endif

	// read 3rd line it should be blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 3rd line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 4th line\r\n");
	#endif

	// read 4th line it should be blank
	len = uart3_readline();
	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 4th line\r\n");
		debug_out("processing line, expected is STATE: <status>\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether __OTHER line
	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
		#endif

		// FIXME: sscanf is not working it is not implemented for strings in minilib-c see stdio.c
		char * __index = memchr(uart3_fifo.line, ':', len);

		// parse STATE: <status> string, i.e we need status string
		if(__index)
		{
			uint8_t __indexlen 	= strlen(__index);
			// +2 for removing :<space>
			memcpy(modem.ipstate, (__index + 2), (__indexlen - 2));

			// remove <cr><lf>
			/*modem.ipstate[__indexlen - 3] 	= '\0';
			modem.ipstate[__indexlen - 4]	= '\0';*/

			#if APPLICATION_LOG_LEVEL == 1
				debug_out("ip state parse success\r\n");
				debug_out("STATUS: ");
				debug_out(modem.ipstate);
				debug_out("\r\n");
			#endif

			// SUCCESS
			flag = 1;
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("ip status parse failed\r\n");
				sprintf(debug_buff, "line: %s", uart3_fifo.line);
				debug_out(debug_buff);
				debug_out("\r\n");
			#endif
		}
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __OTHER, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}
	// FAIL
	return flag;
}

uint8_t gsm_update_rssi(void)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT+CSQ'\r\n");
	#endif

	// send 'AT+CSQ'
	modem_out("AT+CSQ\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT+CSQ' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_DATA)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __DATA line\r\n");
		#endif
		
		char * __next;
		char * __index = index(uart3_fifo.line, ':');

		__index += 2;

		modem.rssi = strtol(__index, &__next, 10);

		// FIXME: Hanging
		/*if(sscanf(uart3_fifo.line, "+CSQ: %d",&rssi))
			modem.rssi = rssi;*/
	}

	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __DATA, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 3rd line\r\n");
	#endif

	// read 3rd line it should be blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 3rd line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 4th line\r\n");
	#endif

	// read 4th line it should be OK
	len = uart3_readline();
	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 4th line\r\n");
		debug_out("processing line, expected is OKr\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether OK line
	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for 'OK'\r\n");
		#endif
		if(strstr(uart3_fifo.line, "OK"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("found ok\r\n");
				debug_out("'AT+CSQ' 'OK' success\r\n");
			#endif
			flag = 1;
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("'OK' not found\r\n");
				sprintf(debug_buff, "line: %s", uart3_fifo.line);
				debug_out(debug_buff);
			#endif
		}
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __OTHER, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	// FAIL
	return flag;
}

uint8_t gsm_set_apn(char * apn)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT+CSTT'\r\n");
	#endif

	// send 'AT'
	modem_out("AT+CSTT=\"");
	modem_out(apn);
	modem_out("\"\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT+CSTT' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for 'OK'\r\n");
		#endif
		if(strstr(uart3_fifo.line, "OK"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("found ok\r\n");
				debug_out("'AT' 'OK' success\r\n");
			#endif
			// SUCCESS
			flag = 1;
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("error expected is __OTHER, but got something else\r\n");
				sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
				debug_out(debug_buff);
			#endif
		}
	}
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("'OK' not found\r\n");
			sprintf(debug_buff, "line: %s", uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	return flag;
}

uint8_t gsm_get_apn(void)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT+CSTT?'\r\n");
	#endif

	// send 'AT+CSQ'
	modem_out("AT+CSTT?\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT+CSTT?' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_DATA)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __DATA line\r\n");
		#endif

		char * __index = memchr(uart3_fifo.line, ':', len);
		char * __comma = memchr(uart3_fifo.line, ',', len);

		// to remove :<space>'"' "
		__index += 3;

		// To remove '"'' "
		__comma -= 1;

		uint8_t _index_len = strlen(__index);
		uint8_t _comma_len = strlen(__comma);

		memcpy(modem.apn, __index , _index_len - _comma_len);
		
		//+CSTT: "TATA.DOCOMO.INTERNET","",""
		// address_start: after 2 from : total_len - len_after_comma

	}

	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __DATA, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 3rd line\r\n");
	#endif

	// read 3rd line it should be blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 3rd line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 4th line\r\n");
	#endif

	// read 4th line it should be OK
	len = uart3_readline();
	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 4th line\r\n");
		debug_out("processing line, expected is OKr\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether OK line
	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for 'OK'\r\n");
		#endif
		if(strstr(uart3_fifo.line, "OK"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("found ok\r\n");
				debug_out("'AT+CSTT?' 'OK' success\r\n");
			#endif
			flag = 1;
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("'OK' not found\r\n");
				sprintf(debug_buff, "line: %s", uart3_fifo.line);
				debug_out(debug_buff);
			#endif
		}
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __OTHER, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	return flag;
}


uint8_t gsm_get_opr_name(void)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT+COPS?'\r\n");
	#endif

	// send 'AT+CSQ'
	modem_out("AT+COPS?\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT+COPS?' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_DATA)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __DATA line\r\n");
		#endif

		char * __index = memchr(uart3_fifo.line, '"', len);

		// to remove :<space>'"' "
		__index += 1;

		// To remove '"'' "
		//__comma -= 1;

		uint8_t _index_len = strlen(__index);

		memcpy(modem.opr, __index , _index_len - 3);

		
		//+COPS: 0,0,"T24"
		// address_start: after 2 from : total_len - len_after_comma

	}

	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __DATA, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 3rd line\r\n");
	#endif

	// read 3rd line it should be blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 3rd line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 4th line\r\n");
	#endif

	// read 4th line it should be OK
	len = uart3_readline();
	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 4th line\r\n");
		debug_out("processing line, expected is OKr\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether OK line
	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for 'OK'\r\n");
		#endif
		if(strstr(uart3_fifo.line, "OK"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("found ok\r\n");
				debug_out("'AT+CSTT?' 'OK' success\r\n");
			#endif
			flag = 1;
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("'OK' not found\r\n");
				sprintf(debug_buff, "line: %s", uart3_fifo.line);
				debug_out(debug_buff);
			#endif
		}
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __OTHER, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}

	return flag;
}

uint8_t gsm_bring_wireless_up(void)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT+CIICR'\r\n");
	#endif

	// send 'AT'
	modem_out("AT+CIICR\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT+CIICR' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for 'OK'\r\n");
		#endif
		if(strstr(uart3_fifo.line, "OK"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("found ok\r\n");
				debug_out("'AT' 'OK' success\r\n");
			#endif
			// SUCCESS
			flag = 1;
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("error expected is __OTHER, but got something else\r\n");
				sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
				debug_out(debug_buff);
			#endif
		}
	}
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("'OK' not found\r\n");
			sprintf(debug_buff, "line: %s", uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}
	// FAIL
	return flag;
}

uint8_t gsm_get_ipaddr(void)
{
	uint16_t len, resp;
	uint8_t flag = 0;

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("sending 'AT+CIFSR'\r\n");
	#endif

	// send 'AT'
	modem_out("AT+CIFSR\r");

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("'AT+CIFSR' sent\r\n");
		debug_out("reading 1st line\r\n");
	#endif

	// Expected first line is blank
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 1st line\r\n");
		debug_out("processing line, expected is blank\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	// check whether blank line
	if(resp == __LINE_BLANK)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __BLANK line\r\n");
		#endif
	}

	// else display error
	// TODO: retry before going to further step
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __BLANK, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}


	#if APPLICATION_LOG_LEVEL == 1
		debug_out("reading 2nd line\r\n");
	#endif

	// read 2nd line
	len = uart3_readline();

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("read 2nd line\r\n");
		debug_out("processing line, expected is __OTHER line\r\n");
	#endif

	resp = process_response(uart3_fifo.line,len);

	if(resp == __LINE_OTHER)
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("got __OTHER line\r\n");
			debug_out("searching for ip\r\n");
		#endif
		if(strstr(uart3_fifo.line, "ERROR"))
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("ip not found error\r\n");
			#endif	
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("ip found error\r\n");
			#endif
			strcpy(modem.ip, uart3_fifo.line);
			flag = 1;
		}

	}
	else
	{
		#if APPLICATION_LOG_LEVEL == 1
			debug_out("error expected is __OTHER, but got something else\r\n");
			sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
			debug_out(debug_buff);
		#endif
	}
	// FAIL
	return flag;
}