
/* data type definitions */
#include "stdint.h"

/* stdlibs */
#include "stdlib.h"

/* driver includes */
#include "uart.h"
#include "misc.h"

/* device includes */
#include "system_LPC17xx.h"
#include "LPC17xx.h"
#include "stdio.h"

/* syscalls */
#include "syscalls.h"

/* string */
#include "string.h"

/* configurations */
#include "FreeRTOSConfig.h"

/* kernel files */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* streams (stdout, stdin, stderr) */
FILE * stdin;
FILE * stdout;
FILE * stderr;

/* protos */
void prvSetupHardware(void);
void debug_out(char *);
void modem_out(char *);
uint8_t process_response(char * ptr, uint16_t len);

/* process */
static void sysboot(void * pvParameters);
static void monModem(void * pvParameters);
static void establishGPRS(void * pvParameters);

xSemaphoreHandle xSemaphore;

char debug_buff[64];

xTaskHandle monTaskHandle, bootTaskHandle, GPRStaskHandle;

int main(void)
{
	/* Setup the Hardware */
	prvSetupHardware();

	xTaskCreate(sysboot,
			(signed portCHAR *)"sysboot",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY,
			&bootTaskHandle);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Never reach here */
	return 0;
}

/* setup hardware */
void prvSetupHardware(void)
{
	SystemInit();
	SystemCoreClockUpdate();
	uart0_init(9600);
	uart3_init(9600);
	debug_out("system started\r\n");
}

/* debug print */
void debug_out(char * ptr)
{
	for(uint8_t i = 0;ptr[i] != '\0';i++)
	{
		uart0_putc(ptr[i]);
	}
}

/* modem print */
void modem_out(char * ptr)
{
	for(uint8_t i = 0;ptr[i] != '\0';i++)
	{
		uart3_putc(ptr[i]);
	}
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

/* DEBUG INFOS */
#define PROCESS_DEBUG_INFO_LEVEL			1
#define APPLICATION_LOG_LEVEL				1

/* TESTS */
#define MALLOC_TEST							1

/* modem response line types */
#define __LINE_BLANK 	0
#define __LINE_DATA		1
#define __LINE_ERROR 	2
#define __LINE_OTHER	3

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

/* processA */
static void sysboot(void * pvParameters)
{
	for(;;)
	{
		uint16_t len;
		xSemaphore = xSemaphoreCreateMutex();

		#if APPLICATION_LOG_LEVEL == 1
				debug_out("system botting\r\n");
				debug_out("semaphore created\r\n");
		#endif

		if(xSemaphore == NULL){
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("semaphore create failed\r\n");
			#endif
		}
		else{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("semaphore create success\r\n");
			#endif
		}

		#if MALLOC_TEST == 1
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("malloc test started\r\n");
			#endif
			char * c;
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("allocating 1024 bytes\r\n");
			#endif
			c = (char *) malloc(1024);
			if(c != NULL)
			{
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("malloc ok\r\n");
				#endif
			}
			else
			{
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("malloc fail\r\n");
				#endif
			}
			#if APPLICATION_LOG_LEVEL == 1 
				debug_out("free 1024 bytes\r\n");
				debug_out("free 1024 bytes success\r\n");
			#endif
			free(c);
		#endif


		#if APPLICATION_LOG_LEVEL == 1
			debug_out("creating tasks\r\n");
		#endif

		// monitor modem
		xTaskCreate(monModem,
				(signed portCHAR *)"monModem",
				configMINIMAL_STACK_SIZE,
				NULL,
				tskIDLE_PRIORITY,
				&monTaskHandle);

		// create task and resume until OK is received from above task
		xTaskCreate(establishGPRS,
				(signed portCHAR *)"GPRS",
				configMINIMAL_STACK_SIZE,
				NULL,
				tskIDLE_PRIORITY,
				&GPRStaskHandle);

		if(monTaskHandle != NULL)
		{
			//vTaskSuspend(monModem);
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("monModem task create success now suspended\r\n");
			#endif			
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("monModem task create error\r\n");
			#endif		
		}

		if(GPRStaskHandle != NULL)
		{
			//vTaskSuspend(GPRStaskHandle);
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("establishGPRS task create success now suspended\r\n");
			#endif			
		}
		else
		{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("establishGPRS task create error\r\n");
			#endif		
		}

		#if APPLICATION_LOG_LEVEL == 1
			debug_out("resuming monModem task\r\n");
		#endif		
		
		// resume the task
		vTaskResume(monModem);

		if( xSemaphoreTake( xSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{

			#if APPLICATION_LOG_LEVEL == 1
				debug_out("returning semaphore\r\n");
			#endif

			#if APPLICATION_LOG_LEVEL == 1
				debug_out("deleting boot task\r\n");
			#endif	

			// return semaphore
			xSemaphoreGive( xSemaphore );

			// delete boot task
			vTaskDelete(bootTaskHandle);	
		}
	}
}

/* monModem */
static void monModem(void * pvParameters)
{
	uint16_t len, resp;
	for(;;)
	{
		// Wait for resource
		if( xSemaphoreTake( xSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
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
				debug_out("got __BLANK line\r\n");
			}

			// else display error
			// TODO: retry before going to further step
			else
			{
				debug_out("error expected is __BLANK, but got something else\r\n");
				sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
				debug_out(debug_buff);
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
				}
				else
				{
					debug_out("error expected is __OTHER, but got something else\r\n");
					sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
					debug_out(debug_buff);
				}
			}
			else
			{
				debug_out("'OK' not found\r\n");
				sprintf(debug_buff, "line: %s", uart3_fifo.line);
				debug_out(debug_buff);
			}

			// Return semaphore
			xSemaphoreGive( xSemaphore );
		}
		vTaskDelay(2000);
	}
}

/* establishGPRS */
static void establishGPRS(void * pvParameters)
{
	uint16_t len, resp;
	for(;;)
	{
		// Wait for resource
		if( xSemaphoreTake( xSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
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
				debug_out("got __BLANK line\r\n");
			}

			// else display error
			// TODO: retry before going to further step
			else
			{
				debug_out("error expected is __BLANK, but got something else\r\n");
				sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
				debug_out(debug_buff);
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
				}
				else
				{
					debug_out("error expected is __OTHER, but got something else\r\n");
					sprintf(debug_buff, "resp: %d, line: %s", resp, uart3_fifo.line);
					debug_out(debug_buff);
				}
			}
			else
			{
				debug_out("'OK' not found\r\n");
				sprintf(debug_buff, "line: %s", uart3_fifo.line);
				debug_out(debug_buff);
			}

			// Return semaphore
			xSemaphoreGive( xSemaphore );
		}
		vTaskDelay(2000);
	}
}

