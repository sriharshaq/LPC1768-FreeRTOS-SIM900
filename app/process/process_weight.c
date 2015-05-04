
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
#include "weight.h"

#define AVG_VAL 50

void process_weight(void * pvParameters)
{
    int32_t _weight;
    int32_t _avg = 0;
    int32_t _last = 0;
    uint8_t sms_flag = 0;
	for(;;)
	{
		if(xSemaphoreTake(weightFlag, (portTickType) 10) == pdTRUE)
		{
            for(uint8_t i = 0;i < AVG_VAL;i++)
            {
                _avg += get_weight();
            }
            xSemaphoreGive(weightFlag);
            _weight = _avg / AVG_VAL;
            _avg = 0;

            if(_weight < __WEIGHT_THRESHOLD && _weight > 0 && sms_flag == 0)
            {
                sms_flag = 1;
                //debug_out("SMS is sending\r\n");
                xQueueSend( smsQueue, ( void * ) &_weight, ( portTickType ) 0 );
            }

            else if(_weight > __WEIGHT_THRESHOLD)
            {
                sms_flag = 0;
            }

            if(_last != _weight)
            {
                _last = _weight;
                xQueueSend( lcdQueue, ( void * ) &_weight, ( portTickType ) 0 );
                xQueueSend( httpQueue, ( void * ) &_weight, ( portTickType ) 0 );
            }

		}
        //vTaskDelay(2);
	}
}