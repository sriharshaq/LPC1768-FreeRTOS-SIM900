
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

void process_connection(void * pvParameters)
{


	xTaskCreate(process_http , 
				( signed char * ) "http", 
				configMINIMAL_STACK_SIZE, 
				( void * ) NULL, 
				tskIDLE_PRIORITY, 
				&httpTaskHandle );

	uint8_t taskFlag = 0;

	vTaskSuspend(httpTaskHandle);

	for(;;)
	{
		if(xSemaphoreTake(modemFlag, (portTickType) 1000 ) == pdTRUE)
		{
			if(gsm_get_tcpstatus())
			{
				xSemaphoreGive(modemFlag);
				strtolower(modem.tcpstatus);
				if(strstr(modem.tcpstatus, "ip start"))
				{
					debug_out("ip start\r\n");
					if(xSemaphoreTake(modemFlag, (portTickType) 1000) == pdTRUE)
					{
						if(gsm_start_gprs())
						{	
							debug_out("gprs started\r\n");
							if(gsm_get_ip_address())
							{
								debug_out("ip address\r\n");
								debug_out(modem.ip_addr);
								xSemaphoreGive(modemFlag);
								if(taskFlag == 0){
									taskFlag = 1;
									vTaskResume(httpTaskHandle);
								}	
							}
						}						
					}
				}
				else if(strstr(modem.tcpstatus, "ip initial"))
				{
					debug_out("ip initial\r\n");
					if(xSemaphoreTake(modemFlag, (portTickType) 1000) == pdTRUE)
					{
						if(gsm_start_gprs())
						{	
							debug_out("gprs started\r\n");
							if(gsm_get_ip_address())
							{
								debug_out("ip address\r\n");
								debug_out(modem.ip_addr);
								xSemaphoreGive(modemFlag);
								if(taskFlag == 0){
									taskFlag = 1;
									vTaskResume(httpTaskHandle);
								}
							}
						}						
					}
				}
				else if(strstr(modem.tcpstatus, "ip config"))
				{
					debug_out("config\r\n");
				}
				else if(strstr(modem.tcpstatus, "ip gprsact"))
				{
					debug_out("gprsact\r\n");
					if(xSemaphoreTake(modemFlag, (portTickType) 1000) == pdTRUE)
					{
						if(gsm_get_ip_address())
						{
							debug_out(modem.ip_addr);
							xSemaphoreGive(modemFlag);
							if(taskFlag == 0){
									taskFlag = 1;
									vTaskResume(httpTaskHandle);
								}
						}
					}					
				}
				else if(strstr(modem.tcpstatus, "ip status"))
				{
					debug_out("ip status\r\n");
					if(xSemaphoreTake(modemFlag, (portTickType) 1000) == pdTRUE)
					{
						if(gsm_get_ip_address())
						{
							debug_out(modem.ip_addr);
							xSemaphoreGive(modemFlag);
							if(taskFlag == 0){
									taskFlag = 1;
									vTaskResume(httpTaskHandle);
								}
						}
					}
				}
				/*else if(strstr(modem.tcpstatus, "tcp connecting"))
				{
					debug_out("connecting\r\n");
				}*/
				else if(strstr(modem.tcpstatus, "tcp connected"))
				{
					debug_out("connected\r\n");
				}
				/*else if(strstr(modem.tcpstatus, "tcp closing"))
				{
					debug_out("closing\r\n");
				}*/
				else if(strstr(modem.tcpstatus, "connect ok"))
				{
					debug_out("connect ok\r\n");
					if(gsm_tcp_disconnect())
					{
						debug_out("disconnect ok\r\n");
						if(gsm_get_ip_address())
						{
							debug_out(modem.ip_addr);
							xSemaphoreGive(modemFlag);
							if(taskFlag == 0){
									taskFlag = 1;
									vTaskResume(httpTaskHandle);
								}
						}
					}
				}
				else if(strstr(modem.tcpstatus, "tcp closed"))
				{
					debug_out("closed\r\n");			
					if(xSemaphoreTake(modemFlag, (portTickType) 1000) == pdTRUE)
					{
						if(gsm_get_ip_address())
						{
							debug_out(modem.ip_addr);
							xSemaphoreGive(modemFlag);
							if(taskFlag == 0){
									taskFlag = 1;
									vTaskResume(httpTaskHandle);
								}
						}
					}
				}
				else if(strstr(modem.tcpstatus, "pdp deact"))
				{
					debug_out("deact\r\n");		
					if(xSemaphoreTake(modemFlag, (portTickType) 1000) == pdTRUE)
					{
						if(gsm_tcp_shutdown())
						{
							debug_out("tcp shutdown\r\n");
							if(detect_and_set_apn())
							{
								debug_out("apn reset\r\n");
								xSemaphoreGive(modemFlag);
							}
						}
					}						
				}
				/*else
				{
					debug_out(modem.tcpstatus);
					debug_out("\r\n");
				}*/
			}			
		}
		vTaskDelay(1000);
	}
}