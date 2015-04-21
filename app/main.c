
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

/* protos */
void prvSetupHardware(void);
void debug_out(char *);
void modem_out(char *);
uint8_t process_response(char * ptr, uint16_t len);

/* process */
static void sysboot(void * pvParameters);
static void monModem(void * pvParameters);
static void establishGPRS(void * pvParameters);
static void monZigbee(void * pvParameters);
static void display(void * pvParameters);

/* semaphores */
xSemaphoreHandle modemUARTSemaphore;
xSemaphoreHandle lcdSemaphore;
xSemaphoreHandle zigbeeSemaphore;

/* Task Handles */
xTaskHandle monTaskHandle, bootTaskHandle, GPRStaskHandle;

/* buffer for debug sprintf */
char debug_buff[64];

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
	uart0_init(9600);							// Debug port

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("debug port is up\r\n");
	#endif

	uart1_init(9600);							// zigbee port
	uart3_init(9600);							// modem port

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("modem and zigbee ports are up\r\n");
	#endif

	lcd_init();									// init lcd

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("lcd init success\r\n");
	#endif

	#if LCD_INIT_PRINT == 1
		lcd_write_instruction_4d(0x01);
		lcd_write_instruction_4d(0x80);
		lcd_print("-hardware init-");
		lcd_write_instruction_4d(0xC0);
		lcd_print("--- success ---");
	#endif

	gsm_allocate_mem();
}


/* processA */
static void sysboot(void * pvParameters)
{
	for(;;)
	{
		uint16_t len;
		modemUARTSemaphore 	= xSemaphoreCreateMutex();
		lcdSemaphore		= xSemaphoreCreateMutex();
		zigbeeSemaphore		= xSemaphoreCreateMutex();

		#if APPLICATION_LOG_LEVEL == 1
				debug_out("system botting\r\n");
				debug_out("semaphores created\r\n");
		#endif

		if(modemUARTSemaphore == NULL){
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("modemUARTSemaphore create failed\r\n");
			#endif
		}
		else{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("modemUARTSemaphore create success\r\n");
			#endif
		}

		if(lcdSemaphore == NULL){
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("lcdSemaphore create failed\r\n");
			#endif
		}
		else{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("lcdSemaphore create success\r\n");
			#endif
		}	


		if(zigbeeSemaphore == NULL){
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("zigbeeSemaphore create failed\r\n");
			#endif
		}
		else{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("zigbeeSemaphore create success\r\n");
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

			if(monTaskHandle != NULL)
			{
				//vTaskSuspend(monModem);
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("monModem task create success\r\n");
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
					debug_out("establishGPRS task create success\r\n");
				#endif			
			}
			else
			{
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("establishGPRS task create error\r\n");
				#endif		
			}

			#if APPLICATION_LOG_LEVEL == 1
				debug_out("returning semaphore\r\n");
			#endif

			#if APPLICATION_LOG_LEVEL == 1
				debug_out("deleting boot task\r\n");
			#endif	

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
			// ping modem
			if(gsm_ping_modem())
			{
				debug_out("ping ok\r\n");
			}

			// update apn
			if(gsm_set_apn("TATA.DOCOMO.INTERNET"))
			{
				debug_out("apn ok\r\n");
			}
			else
				debug_out("apn fail\r\n");

			// Return semaphore
			xSemaphoreGive( modemUARTSemaphore );		
		}
		vTaskDelay(500);
	}
}

/* establishGPRS */
static void establishGPRS(void * pvParameters)
{
	uint16_t len, resp;
	for(;;)
	{
		// Wait for resource
		if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{

			// query ipstatus
			uint8_t state = gsm_update_ipstatus();
			if(state)
			{
				debug_out("ok\r\n");
				if( xSemaphoreTake( lcdSemaphore, ( portTickType ) 1 ) == pdTRUE )
				{
					lcd_write_instruction_4d(0xC0);
					lcd_print(modem.ipstate);
					lcd_print("        ");

					// Return semaphore
					xSemaphoreGive(lcdSemaphore);
				}	
			}

			// query rssi
			state = gsm_update_rssi();
			if(state)
			{
				debug_out("rssi ok\r\n");
				if( xSemaphoreTake( lcdSemaphore, ( portTickType ) 1 ) == pdTRUE )
				{
					sprintf(debug_buff, "%d", modem.rssi);
					lcd_write_instruction_4d(0x80);
					lcd_print(debug_buff);
					lcd_print("        ");

					// Return semaphore
					xSemaphoreGive(lcdSemaphore);
				}	
			}

			// query apn
			state = gsm_get_apn();
			if(state)
			{
				debug_out(modem.apn);
				debug_out("\r\n");
			}

			state = gsm_get_opr_name();
			if(state)
			{
				debug_out(modem.opr);
				debug_out("\r\n");
			}

			state = gsm_bring_wireless_up();
			if(state)
			{
				debug_out("ciicr success\r\n");
			}
			else
			{
				debug_out("ciicr fail\r\n");
			}

			state = gsm_get_ipaddr();
			if(state)
			{
				debug_out("gsm_get_ipaddr success\r\n");
				debug_out(modem.ip);
			}
			else
			{
				debug_out("gsm_get_ipaddr fail\r\n");
			}

			xSemaphoreGive( modemUARTSemaphore );
		}
		vTaskDelay(500);
	}
}

