
/* data type definitions */
#include "stdint.h"

/* stdlibs */
#include "stdlib.h"

/* driver includes */
#include "uart.h"
#include "misc.h"
#include "lcd.h"
#include "delay.h"
#include "gsm.h"
#include "jsmn.h"

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

/* application config */
#include "config.h"

/* streams (stdout, stdin, stderr) */
FILE * stdin;
FILE * stdout;
FILE * stderr;

#define WEIGHT_THRESHOLD 50

/* protos */
void 	prvSetupHardware(void);
uint8_t process_response(char * ptr, uint16_t len);

/* process */
static void sysboot(void * pvParameters);
static void monModem(void * pvParameters);
static void establishGPRS(void * pvParameters);
static void monWeight(void * pvParameters);
static void display(void * pvParameters);

/* semaphores */
xSemaphoreHandle modemUARTSemaphore;
xSemaphoreHandle lcdSemaphore;
xSemaphoreHandle weightSemaphore;

/* Task Handles */
xTaskHandle monTaskHandle, bootTaskHandle, GPRStaskHandle, 
			WeightHandle, displayHandle;

/* buffer for debug sprintf */
char debug_buff[64];

typedef struct 
{
	uint8_t id;
	uint8_t changed;
	int32_t weight;			
}dbType_t;

dbType_t _weight;

volatile uint8_t changed = 0;

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

	uart0_init(9600);							// loadcell port
	uart1_init(9600);							// debug port
	uart3_init(9600);							// modem port

	lcd_init();									// init lcd

	gsm_allocate_mem();
}


/* processA */
static void sysboot(void * pvParameters)
{
	uint8_t state;
	_weight.weight = 0;
	_weight.changed = 0;
	for(;;)
	{
		uint16_t len;
		modemUARTSemaphore 	= xSemaphoreCreateMutex();
		lcdSemaphore		= xSemaphoreCreateMutex();
		weightSemaphore		= xSemaphoreCreateMutex();

		state = gsm_update_ipstatus();
		if(state)
		{
			debug_out("ok\r\n");
		}

		uint8_t state = gsm_get_opr_name();
		if(state)
		{
			debug_out(modem.opr);
			debug_out("\r\n");
		}

		state = gsm_get_apn();
		if(state)
		{
			debug_out(modem.apn);
			debug_out("\r\n");
		}

		uint8_t trials = 0;

		back2:
		if(gsm_set_apn("aircelgprs.pr"))
		{
			debug_out("apn ok\r\n");
		}
		else
		{
			trials++;
			if(trials < 10)
			{
				vTaskDelay(100);
				goto back2;
			}
			debug_out("apn fail\r\n");
		}

		state = gsm_get_apn();
		if(state)
		{
			debug_out(modem.apn);
			debug_out("\r\n");
		}

		state = gsm_update_ipstatus();
		if(state)
		{
			debug_out("ip status update\r\n");
		}

		state = gsm_bring_wireless_up();
		if(state)
		{
			debug_out("wireless is up\r\n");
		}
		else
		{
			debug_out("wireless is fail\r\n");
		}

		state = gsm_update_ipstatus();
		if(state)
		{
			debug_out("ip status update\r\n");
		}



		if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
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

			xTaskCreate(display,
					(signed portCHAR *)"display",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&displayHandle);	

			xTaskCreate(monWeight,
					(signed portCHAR *)"weight",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&WeightHandle);	

			// return semaphore
			xSemaphoreGive( modemUARTSemaphore );
			xSemaphoreGive(lcdSemaphore);

			// delete boot task
			vTaskDelete(bootTaskHandle);	
		}
	}
}

/* monModem */
static void monModem(void * pvParameters)
{
	for(;;)
	{
		// Wait for resource
		if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
			xSemaphoreGive( modemUARTSemaphore );		
		}
		vTaskDelay(500);
	}
}

/* monWeight */
static void monWeight(void * pvParameters)
{
	int32_t last_read = 0;
	changed = 0;
	for(;;)
	{
		char * __next;
		char * __last;

		if( xSemaphoreTake( weightSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
			uart0_readline();
			int32_t __weight0 = strtol(uart0_fifo.line, &__next, 10);
			int32_t __weight1 = strtol(__next + 1, &__last, 10);
			_weight.weight = (__weight0 * 1000) + (__weight1);

			if(last_read < _weight.weight)
			{
				if( (_weight.weight - WEIGHT_THRESHOLD) > last_read)
				{
					_weight.changed = 1;
					changed = 1;
					last_read = _weight.weight;
					debug_out("changed 1\r\n");
				}
				/*else
					_weight.changed = 0;*/
			}
			else if(last_read > _weight.weight)
			{
				if( (last_read - WEIGHT_THRESHOLD) > _weight.weight)
				{
					_weight.changed = 1;
					changed = 1;
					last_read = _weight.weight;
					debug_out("changed 2\r\n");
				}
				/*else
					_weight.changed = 0;*/
			}
			else
			{
				//sprintf(debug_buff, "%d,%d\r\n", last_read, _weight.weight);
				//debug_out(debug_buff);
				//_weight.changed = 0;
			}
			xSemaphoreGive(weightSemaphore);
		}
	}
}

static void display(void * pvParameters)
{
	uint8_t count = 0;
	uint8_t state;
	char buff[20];
	for(;;)
	{
		if( xSemaphoreTake( lcdSemaphore, ( portTickType ) 1 ) == pdTRUE )
		{
			if(count == 0)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print("weight: ");
				lcd_print("            ");
				sprintf(buff, " %d g", _weight.weight);
				lcd_write_instruction_4d(0xC0);
				lcd_print(buff);
				lcd_print("              ");
				count = 1;
			}
			
			else if(count == 1)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print("operator:  ");
				lcd_print("            ");
				lcd_write_instruction_4d(0xC0);
				lcd_print(modem.opr);
				lcd_print("             ");
				count = 2;
			}

			else if(count == 2)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print("Access Point");
				lcd_print("            ");
				lcd_write_instruction_4d(0xC0);
				lcd_print(modem.apn);
				lcd_print("             ");
				count = 3;
			}

			else if(count == 3)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print("GPRS");
				lcd_print("            ");
				lcd_write_instruction_4d(0xC0);
				lcd_print(modem.ipstate);
				lcd_print("             ");
				count = 4;
			}

			else if(count == 4)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print("IP");
				lcd_print("                 ");
				lcd_write_instruction_4d(0xC0);
				lcd_print(modem.ip);
				lcd_print("             ");
				count = 0;
			}	
			xSemaphoreGive(lcdSemaphore);		
		}

		if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
			state = gsm_update_ipstatus();
			state = gsm_get_ipaddr();
			xSemaphoreGive( modemUARTSemaphore );
		}
		vTaskDelay(750);
	}
}



/* establishGPRS */
static void establishGPRS(void * pvParameters)
{
	uint16_t len, resp;
	uint8_t ref = 0;
	uint8_t state;
	char buff[32];
	for(;;)
	{
		// if(_weight.changed == 1)
		// {
			while(changed == 0);
			debug_out("weight changed http put started\r\n");
			changed = 0;

			// Wait for resource
			if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
			{
				sprintf(buff, "{\"w_val\":%d}",_weight.weight);

				state = gsm_http_put("lit-taiga-2854.herokuapp.com","/weight/1",buff);

				if(state || strstr(modem.ipstate,"CONNECT OK"))
				{
					debug_out("http ok\r\n");

					len = uart3_readline();
					debug_out(uart3_fifo.line);

					debug_out("header\r\n");
					do
					{
						len = uart3_readline();
					}
					while(isblankstr(uart3_fifo.line, len) != 1);

					debug_out("data enter\r\n");
					
					uint16_t _i = 0;
					while(uart3_fifo.num_bytes > 0)
					{
						char c = uart3_getc();
						modem.httpdata[_i++] = c;
						vTaskDelay(1);
					}
					debug_out("data\r\n");
					modem.httpdata[_i] = '\0';
					debug_out(modem.httpdata);

					uint8_t trials = 0;
					back1:
					state = gsm_tcp_close();
					if(state)
					{
						debug_out("TCP close\r\n");
					}
					else
					{
						debug_out("TCP close fail\r\n");
						trials++;
						if(trials < 10)
						{
							vTaskDelay(100);
							goto back1;
						}
					}
				}
				else
				{
					debug_out("http fail\r\n");
				}
				xSemaphoreGive( modemUARTSemaphore );
			}
		//}

			
	}
}



