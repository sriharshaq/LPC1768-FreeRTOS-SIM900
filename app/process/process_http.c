
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


void process_http(void * pvParameters)
{
	debug_out("http started\r\n");
	uint8_t count = 1;
	char path[32];
	char buff[20];

	device * __device;
	for(;;)
	{
		if(xSemaphoreTake(modemFlag, (portTickType) 10 ) == pdTRUE)
		{
			if(gsm_get_tcpstatus())
			{
				strtolower(modem.tcpstatus);
				if(	strstr(modem.tcpstatus, "ip status") 	|| 
					strstr(modem.tcpstatus, "tcp closed") 	|| 
					strstr(modem.tcpstatus, "connect ok") 	|| 
					strstr(modem.tcpstatus, "ip gprsact")
				  )
				{
					debug_out("http get\r\n");
					//count++;

					bzero(path,30);
					sprintf(buff, "%d", count);
					strcpy(path, BASE);
					strcat(path, buff);

					if(gsm_http_get(URL,path))
					{
						debug_out("connected\r\n");
						http_read_data();

						debug_out("data\r\n");
						debug_out(modem.httpdata);

						if(gsm_tcp_disconnect())
						{
							debug_out("tcp closed\r\n");
							xSemaphoreGive(modemFlag);
						}							

						if(strlen(modem.httpdata) > 0)
						{
							char * __index  = strstr(modem.httpdata, "\"d_state\":");
							if(__index)
							{
								char * next;
								__index += sizeof("\"d_state\"");
								uint8_t result = strtol(__index, &next, 10);
								sprintf(buff, "state: %d\r\n", result);
								debug_out(buff);

								__device = &_device;

								__device->deviceid 		= count;
								__device->devicestate 	= result;
								xQueueSend( lcdQueue, ( void * ) &__device, ( portTickType ) 0 );
								xQueueSend( xbeeQueue, ( void * ) &__device, ( portTickType ) 0 );
							}
						}

					}

					if(count >= 3)
					{
						count = 1;
					}
					else
						count++;							
				}
			}
		}
		else
		{
			modem_flush_rx();
			vTaskDelay(1000);
		}
	}
}