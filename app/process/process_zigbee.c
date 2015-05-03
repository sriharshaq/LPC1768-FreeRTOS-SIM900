
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

const char addrH[] = "13A200";
const char * _xbeeAddrList[] = {"40DC8B6D", "40BF8CA8", "40DC8B76"};

void process_zigbee(void * pvParameters)
{
	device * __device;
	char addr[20];
	char dat[10];
	for(;;)
	{
		if(xSemaphoreTake(zigbeeFlag, (portTickType) 10) == pdTRUE)
		{
			if( xbeeQueue != 0 )
    		{
        		if( xQueueReceive( xbeeQueue, &( __device ), ( portTickType ) 10 ) )
        		{
        			strcpy(addr,_xbeeAddrList[__device->deviceid - 1]);
        			
        			if(__device->devicestate == 1)
        			{
        				dat[0] = 'a';
        				dat[1] = '\0';
        			}
        			else if(__device->devicestate == 0)
        			{
        				dat[0] = 'b';
        				dat[1] = '\0';
        			}
        			send_to_end_device(addrH,addr, dat);

        			if(__device->deviceid == 1)
        			{
        				if(__device->devicestate == 1)
        				{
        					LPC_GPIO1->FIOCLR |= (1 << 19);
        				}
        				else
        				{
        					LPC_GPIO1->FIOSET |= (1 << 19);
        				}
        			}
        			else if(__device->deviceid == 2)
        			{
        				if(__device->devicestate == 1)
        				{
        					LPC_GPIO1->FIOCLR |= (1 << 20);
        				}
        				else
        				{
        					LPC_GPIO1->FIOSET |= (1 << 20);
        				}
        			}
         			else if(__device->deviceid == 3)
        			{
        				if(__device->devicestate == 1)
        				{
        					LPC_GPIO1->FIOCLR |= (1 << 21);
        				}
        				else
        				{
        					LPC_GPIO1->FIOSET |= (1 << 21);
        				}
        			}
       			}
       		}
			xSemaphoreGive(zigbeeFlag);
		}
	}
}