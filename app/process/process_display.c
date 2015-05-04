
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
	int32_t _weight;
	uint8_t msg;
	char 	buff[20];
	int32_t __weight = -111;
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
				case 5: if(__weight != -111){
							sprintf(buff, "%d g", _weight);
							lcd_set_xy(0,0);
							lcd_print("weight",1);
							lcd_set_xy(0,1);
							lcd_print(buff,1);
							break;
				}
						

			}
			xSemaphoreGive(lcdFlag);
			count++;
			if(count > 5){
				count = 0;	
			}

			if( lcdQueue != 0 )
    		{
        		if( xQueueReceive( lcdQueue, &( _weight ), ( portTickType ) 10 ) )
        		{
        			__weight = _weight;
        			sprintf(buff, "%d", _weight);
        			lcd_set_xy(0,0);
        			lcd_print("    WEIGHT   ", 1);
        			lcd_set_xy(0,1);
        			lcd_print(buff, 0);
        			lcd_write_character_4d(' g');
        			lcd_print("                     ", 0);
       			}
       		}

			if( msgQueue != 0 )
    		{
        		if( xQueueReceive( msgQueue, &( msg ), ( portTickType ) 10 ) )
        		{
        			lcd_set_xy(0,0);
        			lcd_print("    INFO     ", 1);
        			lcd_set_xy(0,1);
        			if(msg == 0)
        			{
        				lcd_print("http started", 1);
        			}
        			else if(msg == 1)
        			{
        				lcd_print("http ok", 1);
        			}
        			if(msg == 2)
        			{
        				lcd_print("tcp disconnect", 1);
        			}
        			if(msg == 3)
        			{
        				lcd_print("sending sms", 1);
        			}
        			if(msg == 4)
        			{
        				lcd_print("sms sent", 1);
        			}
       			}
       		}
			vTaskDelay(300);		
		}	
		vTaskDelay(1);
	}
}