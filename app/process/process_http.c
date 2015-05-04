
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
	char dat[50];
	uint8_t msg;
	int32_t _weight;
	for(;;)
	{
		if( xQueueReceive( httpQueue, &( _weight ), ( portTickType ) 10 ) )
		{
			if(xSemaphoreTake(modemFlag, (portTickType) 1000) == pdTRUE)
			{	
				/* send to http */
				if(gsm_get_tcpstatus())
				{
					strtolower(modem.tcpstatus);
					if(	strstr(modem.tcpstatus, "ip status") 	|| 
						strstr(modem.tcpstatus, "tcp closed") 	|| 
						strstr(modem.tcpstatus, "connect ok") 	|| 
						strstr(modem.tcpstatus, "ip gprsact")
					  )
					{
						debug_out("http put\r\n");
						bzero(dat,50);
						sprintf(dat, "{\"w_val\":%d}", _weight);

						msg = 0;
						xQueueSend( msgQueue, ( void * ) &msg, ( portTickType ) 0 );

						if(gsm_http_put(URL, BASE, dat))
						{
							msg = 1;
							xQueueSend( msgQueue, ( void * ) &msg, ( portTickType ) 0 );
							debug_out("http put ok\r\n");
							http_read_data();
							debug_out(modem.httpdata);
							if(gsm_tcp_disconnect())
							{
								msg = 2;
								xQueueSend( msgQueue, ( void * ) &msg, ( portTickType ) 0 );
								debug_out("tcp closed\r\n");
								xSemaphoreGive(modemFlag);
							}							
						}
					}
				}
			}
		}
	}
}
