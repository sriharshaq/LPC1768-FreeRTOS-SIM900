
#ifndef __app_h__
#define __app_h__

#define URL 	"lit-taiga-2854.herokuapp.com"
#define BASE	"/zigbee/"

typedef struct
{
	uint8_t deviceid;
	uint8_t devicestate;
}device;

/* task handles */
extern xTaskHandle 		connectionTaskHandle;
extern xTaskHandle 		displayTaskHandle;
extern xTaskHandle 		zigbeeTaskHandle;
extern xTaskHandle 		httpTaskHandle;

/* semaphores */
extern xSemaphoreHandle 	lcdFlag;
extern xSemaphoreHandle	modemFlag;
extern xSemaphoreHandle	zigbeeFlag;

extern xQueueHandle lcdQueue;
extern xQueueHandle xbeeQueue;

extern device * _device;

extern void process_connection(void * pvParameters);
extern void process_display(void * pvParameters);
extern void process_http(void * pvParameters);
extern void process_zigbee(void * pvParameters);

#endif