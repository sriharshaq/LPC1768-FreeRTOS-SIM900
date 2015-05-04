
#ifndef __app_h__
#define __app_h__

#define URL 	"lit-taiga-2854.herokuapp.com"
#define BASE	"/weight/1"

#define __WEIGHT_THRESHOLD 500


/* task handles */
extern xTaskHandle 		connectionTaskHandle;
extern xTaskHandle 		displayTaskHandle;
extern xTaskHandle 		weighingScaleTaskHandle;
extern xTaskHandle 		httpTaskHandle;
extern xTaskHandle 		smsTaskHandle;

/* semaphores */
extern xSemaphoreHandle lcdFlag;
extern xSemaphoreHandle	modemFlag;
extern xSemaphoreHandle	weightFlag;

extern xQueueHandle lcdQueue;
extern xQueueHandle httpQueue;
extern xQueueHandle smsQueue;
extern xQueueHandle msgQueue;

extern void process_connection(void * pvParameters);
extern void process_display(void * pvParameters);
extern void process_http(void * pvParameters);
extern void process_weight(void * pvParameters);
extern void process_sms(void * pvParameters);

#endif