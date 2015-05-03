
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

void process_display(void * pvParameters)
{
	uint8_t count = 0;
	device * __device;
	for(;;)
	{
		if(xSemaphoreTake(lcdFlag, (portTickType) 10 ) == pdTRUE)
		{
			switch(count)
			{
				case 0: lcd_set_xy(0,0);
						lcd_print("operator",1);
						lcd_set_xy(0,1);
						lcd_print(modem.operator_name,1);
						break;
				case 1: lcd_set_xy(0,0);
						lcd_print("access point",1);
						lcd_set_xy(0,1);
						lcd_print(modem.getapn,1);
						break;
				case 2: lcd_set_xy(0,0);
						lcd_print("signal strength",1);
						lcd_set_xy(0,1);
						if(modem.rssi == 0)
							lcd_print("-115 dBm or less",1);
						else if(modem.rssi == 1)
							lcd_print("-111 dBm",1);
						else if( (modem.rssi >= 2) && (modem.rssi <= 30) )
							lcd_print("-110... -54 dBm",1);
						else if( modem.rssi == 31 )
							lcd_print("-52 dBm or greater",1);
						else
							lcd_print("not known",1);	
						break;
				case 3: lcd_set_xy(0,0);
						lcd_print("tcp status",1);
						lcd_set_xy(0,1);
						lcd_print(modem.tcpstatus,1);
						break;
				case 4: lcd_set_xy(0,0);
						lcd_print("ip address",1);
						lcd_set_xy(0,1);
						lcd_print(modem.ip_addr,1);
						break;
				/*case 5: lcd_set_xy(0,0);
						lcd_print("xbee mac addr",1);
						lcd_set_xy(0,1);
						lcd_print(modem.ip_addr,1);
						break;*/

			}
			xSemaphoreGive(lcdFlag);
			count++;
			if(count > 4){
				count = 0;	
			}

			if( lcdQueue != 0 )
    		{
    			//__device = &_device;
        		if( xQueueReceive( lcdQueue, &( __device ), ( portTickType ) 10 ) )
        		{
        			lcd_set_xy(0,0);
        			lcd_print("device: ", 1);
        			lcd_set_xy(sizeof("device: "), 0);
        			lcd_write_character_4d(__device->deviceid + 48);
        			lcd_set_xy(0,1);
        			lcd_print("status: ", 1);
        			lcd_set_xy(sizeof("status: "), 1);
        			lcd_write_character_4d(__device->devicestate + 48);
       			}
       		}
			vTaskDelay(1000);		
		}	
		vTaskDelay(1);
	}
}