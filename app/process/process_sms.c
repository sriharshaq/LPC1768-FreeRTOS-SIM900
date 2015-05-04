
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

void process_sms(void * pvParameters)
{
	int32_t _weight = 0;
	char buff[50];
	uint8_t msg;
	for(;;)
	{
		if( xQueueReceive( smsQueue, &( _weight ), ( portTickType ) 1000 ) )
		{
			debug_out("sms enter\r\n");
			if(xSemaphoreTake(modemFlag, (portTickType) 100000) == pdTRUE)
			{
				debug_out("sending\r\n");
				sprintf(buff, "container A weight is %d grams", _weight);

				msg = 3;
				xQueueSend( msgQueue, ( void * ) &msg, ( portTickType ) 0 );
				/* send sms */
				if(gsm_send_sms(__NUM, buff))
				{
					debug_out("sms sent\r\n");
					msg = 4;
					xQueueSend( msgQueue, ( void * ) &msg, ( portTickType ) 0 );

					vTaskDelay(2);
					modem_flush_rx();
					xSemaphoreGive(modemFlag);
				}
			}			
		}
	}
}