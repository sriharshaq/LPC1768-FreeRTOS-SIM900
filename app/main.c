
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

/* Define how many number of operator or apn we have (opr and apn array length should be same) */
#define APN_OPR_LIST_LEN 10

/* Setup Hardware Function */
void 	prvSetupHardware(void);

/* Tasks */
static void systemBoot(void *);
static void connectGPRS(void *);
static void updateModemStatus(void *);
static void displayProcess(void *);
static void measureWeight(void *);

/* Semaphores */
xSemaphoreHandle modemSema;
xSemaphoreHandle displaySema;
xSemaphoreHandle measureSema;

/* Task Handles */
xTaskHandle systemBootHandle;
xTaskHandle connectGPRSHandle;
xTaskHandle updateModemStatusHandle;
xTaskHandle displayProcessHandle;
xTaskHandle measureWeightHandle;

const char * oprList[]	=	{	
								"airtel",
								"cellone",
								"idea",
								"aircel",
								"tata docomo",
								"reliance",
								"vodafone"
							};

const char * apnList[]	=	{	
								"airtel",
								"cellone",
								"idea",
								"aircel",
								"tata docomo",
								"reliance",
								"vodafone"
							};

/** @brief 
 *  @param 
 *  @return 
 */
void prvSetupHardware(void)
{
	SystemInit();
	SystemCoreClockUpdate();

	/* init uart's */
	uart0_init(__UART0_BAUDRATE);				// loadcell port
	uart1_init(__UART1_BAUDRATE);				// debug port
	uart3_init(__UART3_BAUDRATE);				// modem port

	/* initiate lcd */
	lcd_init();									// init lcd

	/* init buffers for gsm modem */
	gsm_buff_init();
}


/** @brief 
 *  @param 
 *  @return 
 */
int main(void)
{
	/* Setup the Hardware */
	prvSetupHardware();

	/* Create Boot Task */
	xTaskCreate(	systemBoot,
					(signed portCHAR *)"boot",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&systemBootHandle);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Never reach here */
	return 0;
}

/** @brief 
 *  @param 
 *  @return 
 */
static void systemBoot(void * pvParameters)
{
	uint8_t __response;
	for(;;)
	{
		/* create semaphores */
		modemSema 			= xSemaphoreCreateMutex();
		displaySemap		= xSemaphoreCreateMutex();
		measureSema			= xSemaphoreCreateMutex();

		/* Get operator name */
		// TODO: process return value
		__response = gsm_get_operator();

		/* convert to lower for comparison */
		strtolower(modem.operator);

		/* check operator and findout APN */
		for(uint8_t i = 0;i < APN_OPR_LIST_LEN;i++)
		{
			if(strstr(modem.operator, oprList[i]))
			{
				strcpy(modem.setapn, apnList[i]);
				break;
			}
		}

		/* delete the boot task */
		vTaskDelete(systemBootHandle);

	}
}


static void connectGPRS(void *)
{
	for(;;)
	{

		/* It should wait for queue */

		/* Check IP Status and check whether shutdown is required */

		/* Get TCP Status */
		// TODO: process return value
		__response = gsm_get_tcpstatus();

		/* lower case the state for comparison */
		strtolower(modem.tcpstatus);



		/* Check with predefined APN */
		if(strstr(modem.getapn, modem.setapn) == NULL)
		{
			/* If both are not matched set access point */
			// TODO: process return value
			__response = gsm_set_accesspoint(modem);
		}



		/* Check different state and do what the action required */

		if(gsm_start_gprs())
		{
			if(gsm_get_ip_address())
			{
				gsm_get_operator();
				gsm_get_tcpstatus();
			}
		}

		/* get access point name */
		// TODO: process return value
		__response = gsm_get_accesspoint(modem);
	}
}



static void updateModemStatus(void *)
{
	for(;;)
	{

	}	
}
static void displayProcess(void *)
{
	for(;;)
	{

	}	
}
static void measureWeight(void *)
{
	for(;;)
	{
		/* Wait for UART semaphore */
		if( xSemaphoreTake( measureSema, (portTickType) 10) == pdTRUE)
		{
			/* Got resource */

			/* Read the line */

		}
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



