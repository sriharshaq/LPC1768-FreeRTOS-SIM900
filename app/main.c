
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
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "config.h"
#include "app.h"

/* streams (stdout, stdin, stderr) */
FILE * stdin;
FILE * stdout;
FILE * stderr;

extern uint8_t system_boot(void);

/* task handles */
xTaskHandle 		connectionTaskHandle;
xTaskHandle 		displayTaskHandle;
xTaskHandle 		weighingScaleTaskHandle;
xTaskHandle 		httpTaskHandle;
xTaskHandle 		smsTaskHandle;

/* semaphores */
xSemaphoreHandle 	lcdFlag;
xSemaphoreHandle	modemFlag;
xSemaphoreHandle	weightFlag;

/* Queues */
xQueueHandle lcdQueue;
xQueueHandle smsQueue;
xQueueHandle httpQueue;
xQueueHandle msgQueue;


int main(void)
{
	if(system_boot())
	{

		/* create semaphores */
		lcdFlag 	= xSemaphoreCreateMutex();
		modemFlag 	= xSemaphoreCreateMutex();
		weightFlag	= xSemaphoreCreateMutex();

		xTaskCreate(process_connection , 
					( signed char * ) "connection", 
					configMINIMAL_STACK_SIZE, 
					( void * ) NULL, 
					tskIDLE_PRIORITY, 
					&connectionTaskHandle );

		xTaskCreate(process_display , 
					( signed char * ) "display", 
					configMINIMAL_STACK_SIZE, 
					( void * ) NULL, 
					tskIDLE_PRIORITY, 
					&displayTaskHandle );

		xTaskCreate(process_weight , 
					( signed char * ) "weight", 
					configMINIMAL_STACK_SIZE, 
					( void * ) NULL, 
					tskIDLE_PRIORITY, 
					&weighingScaleTaskHandle );

		lcdQueue  	= xQueueCreate(10, sizeof(int32_t));
		httpQueue 	= xQueueCreate(10, sizeof(int32_t));
		smsQueue 	= xQueueCreate(10, sizeof(int32_t));
		msgQueue	= xQueueCreate(5, sizeof(uint8_t));

		/* start the scheduler */
		vTaskStartScheduler();	
		
		/* we never reach here */
		return 0;	
	}

	lcd_set_xy(0,0);
	lcd_print("going idle",1);
	for(;;){
		;
	}
	/* we never reach here */
	return 0;
}




