
/* data type definitions */
#include "stdint.h"

/* stdlibs */
#include "stdlib.h"

/* driver includes */
#include "uart.h"
#include "misc.h"
#include "lcd.h"
#include "delay.h"
#include "gsm.h"

/* device includes */
#include "system_LPC17xx.h"
#include "LPC17xx.h"
#include "stdio.h"

/* syscalls */
#include "syscalls.h"

/* string */
#include "string.h"

/* configurations */
#include "FreeRTOSConfig.h"

/* kernel files */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* application config */
#include "config.h"

#define __NUM "9342833087"

/* streams (stdout, stdin, stderr) */
FILE * stdin;
FILE * stdout;
FILE * stderr;

#define WEIGHT_THRESHOLD	100

/* Define how many number of operator or apn we have (opr and apn array length should be same) */
#define APN_OPR_LIST_LEN 8

/* Setup Hardware Function */
void 	prvSetupHardware(void);

/* Tasks */
static void systemBoot(void * pvParameters);
static void connectGPRS(void * pvParameters);
static void updateModemStatus(void * pvParameters);
static void displayProcess(void * pvParameters);
static void measureWeight(void * pvParameters);
static void httpProc(void * pvParameters);
static void sendSms(void * pvParameters);

/* Semaphores */
xSemaphoreHandle modemSema;
xSemaphoreHandle displaySema;
xSemaphoreHandle measWeightSema;

#ifdef __DEBUG_MESSAGES__
	xSemaphoreHandle debugSema;
#endif

/* Task Handles */
xTaskHandle systemBootHandle;
xTaskHandle connectGPRSHandle;
xTaskHandle updateModemStatusHandle;
xTaskHandle displayProcessHandle;
xTaskHandle measWeightHandle;
xTaskHandle httpTaskHandle;
xTaskHandle smsHandle;

/* Queues */
xQueueHandle httpQueue;
xQueueHandle smsQueue;
xQueueHandle lcdQueue;

uint32_t weight_val = 0;

/* List of operators */
const char * oprList[]	=	{	
								"airtel",
								"cellone",
								"idea",
								"aircel",
								"tata docomo",
								"t24",
								"reliance",
								"vodafone"
							};

/* List of Access Points for different operator */
const char * apnList[]	=	{	
								"airtelgprs.com",
								"bsnlnet",
								"internet",
								"aircelgprs.pr",
								"TATA.DOCOMO.INTERNET",
								"TATA.DOCOMO.INTERNET",
								"rcomnet",
								"www"
							};

/** @brief 
 *  @param 
 *  @return 
 */
void prvSetupHardware(void)
{
	SystemInit();
	SystemCoreClockUpdate();

	/* init uart's */
	uart0_init(__UART0_BAUDRATE);				// RFID Reader port
	uart1_init(__UART1_BAUDRATE);				// debug port
	uart3_init(__UART3_BAUDRATE);				// modem port

	/* initiate lcd */
	lcd_init();									// init lcd

	/* init buffers for gsm modem */
	gsm_buff_init();
}


/** @brief 
 *  @param 
 *  @return 
 */
int main(void)
{
	/* Setup the Hardware */
	prvSetupHardware();

	#ifdef __DEBUG_MESSAGES__
		debug_out("system started\r\n");
		debug_out("hardware setup completed\r\n");
		debug_out("creating the tasks\r\n");
	#endif

	/* Create Boot Task */
	xTaskCreate(	systemBoot,
					(signed portCHAR *)"boot",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&systemBootHandle);

	#ifdef __DEBUG_MESSAGES__
		debug_out("boot task created\r\n");
		debug_out("starting the os\r\n");
	#endif

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Never reach here */
	return 0;
}

/** @brief 
 *  @param 
 *  @return 
 */
static void systemBoot(void * pvParameters)
{
	int8_t __response;
	for(;;)
	{
		/* create semaphores */
		modemSema 			= xSemaphoreCreateMutex();
		displaySema			= xSemaphoreCreateMutex();
		measWeightSema		= xSemaphoreCreateMutex();
		debugSema			= xSemaphoreCreateMutex();

		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("semaphore created\r\n");
				debug_out("ping modem\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		for(uint8_t i = 0;i < 8;i++)
		{
			if(gsm_ping_modem())
			{
				debug_out("PING: ");
				debug_putc(i + 48); 
				debug_out("\r\n");
			}	
		}

		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("reading operator name from modem\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		/* Get operator name */
		// TODO: process return value
		__response = gsm_get_operator_name(&modem);

		/* convert to lower for comparison */
		strtolower(modem.operator_name);

		/* find out apn */
		for(uint8_t i = 0;i < APN_OPR_LIST_LEN;i++)
		{
			if(strstr(modem.operator_name, oprList[i]))
			{
				strcpy(modem.setapn, apnList[i]);
				break;
			}
		}

		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("apn search completed\r\n");
				debug_out("access point name for operator ");
				debug_out(modem.operator_name);
				debug_out(" is ");
				debug_out(modem.setapn);
				debug_out("\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("reading apn from modem\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		/* read apn from modem */
		__response = gsm_get_accesspoint(&modem);

		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("read apn completed\r\n");
				debug_out("access point read from modem: ");
				debug_out(modem.getapn);
				debug_out("\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		/* Check with predefined APN */
		if(strstr(modem.getapn, modem.setapn) == NULL)
		{
			/* If both are not matched set access point */
			// TODO: process return value
			//__response = gsm_set_accesspoint(&modem);
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("access point read from modem is different from actual\r\n");
					debug_out("So setting actual accesspoint name\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif

			__response = gsm_set_accesspoint(&modem);

			if(__response)
			{
				#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("set accesspoint name success\r\n");
						debug_out("reading apn again from modem\r\n");
						xSemaphoreGive(debugSema);
					}
				#endif		

				__response = gsm_get_accesspoint(&modem);
				#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("read apn completed\r\n");
						debug_out("apn name read: ");
						debug_out(modem.getapn);
						debug_out(", actual: ");
						debug_out(modem.setapn);
						debug_out("\r\n");
						xSemaphoreGive(debugSema);
					}
				#endif					
			}
		}

		else
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("modem has valid apn no need to set\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif			
		}

		httpQueue 	= xQueueCreate( 10, sizeof(uint32_t) );
		smsQueue 	= xQueueCreate( 10, sizeof(uint32_t) );
		lcdQueue 	= xQueueCreate( 10, sizeof(uint32_t) );

		/* Create GPRS Task */
		xTaskCreate(	connectGPRS,
						(signed portCHAR *)"gprs",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&connectGPRSHandle);

		/* Create GPRS Task */
		xTaskCreate(	httpProc,
						(signed portCHAR *)"gprs",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&httpTaskHandle);

		/* Create GPRS Task */
		xTaskCreate(	measureWeight,
						(signed portCHAR *)"rfid",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&measWeightHandle);

		/*xTaskCreate(	sendSms,
						(signed portCHAR *)"sendsms",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&smsHandle);*/
		
		xTaskCreate(	displayProcess,
						(signed portCHAR *)"display",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&displayProcessHandle);

		xSemaphoreGive(measWeightSema);

		/* delete the boot task */
		vTaskDelete(systemBootHandle);

	}
}


static void connectGPRS(void * pvParameters)
{
	int8_t __response;

	#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("GPRS task started\r\n");
				xSemaphoreGive(debugSema);
			}
	#endif

	for(;;)
	{
		/* Check IP Status and check whether shutdown is required */
		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("reading tcp status from modem\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		/* Get TCP Status */
		__response = gsm_get_tcpstatus(&modem);

		/* lower case the state for comparison */
		strtolower(modem.tcpstatus);

		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("reading tcp status completed\r\n");
				debug_out("status: ");
				debug_out(modem.tcpstatus);
				debug_out("\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		/* check tcpstatus and do what action is required to bring wireless up */
		if(strstr(modem.tcpstatus, "ip start") || strstr(modem.tcpstatus, "ip initial"))
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("ip start state bringing wireless up\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif				
			if(gsm_start_gprs())
			{
				#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("GPRS start success\r\n");
						debug_out("reading ip address\r\n");
						xSemaphoreGive(debugSema);
					}
				#endif	
				if(gsm_get_ip_address(&modem))
				{
					#ifdef __DEBUG_MESSAGES__
						if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
						{
							debug_out("Read IP address success\r\n");
							debug_out(modem.ip_addr);
							xSemaphoreGive(debugSema);
						}
					#endif	
				}
			}
		}
		else if(strstr(modem.tcpstatus, "ip config"))
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("ip configure\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif				
		}
		else if (	strstr(modem.tcpstatus, "ip gprsact") 	|| 
					strstr(modem.tcpstatus, "ip status") 	||
					strstr(modem.tcpstatus, "tcp closed")
				)
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("gprs act\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif	

			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("reading ip address\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif	
			if(gsm_get_ip_address(&modem))
			{
				#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("Read IP address success\r\n");
						debug_out(modem.ip_addr);
						xSemaphoreGive(debugSema);
					}
				#endif	
			}
		}
		else if(strstr(modem.tcpstatus, "tcp connecting"))
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("connecting\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif				
		}
		else if(strstr(modem.tcpstatus, "connect ok"))
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("connect ok\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif	
			if(gsm_tcp_disconnect())
			{
				#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("disconnect OK\r\n");
					xSemaphoreGive(debugSema);
				}
				#endif	
			}			
		}
		else if(strstr(modem.tcpstatus, "tcp closing"))
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("tcp closing\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif				
		}
		else if(strstr(modem.tcpstatus, "pdp deact"))
		{
				// FIXME: What to do?
				#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("pdp deactivated shutdown is required\r\n");
					xSemaphoreGive(debugSema);
				}
			#endif			
		}

		#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("task is no more required suspending\r\n");
				xSemaphoreGive(debugSema);
			}
		#endif

		/* suspend the task */
		vTaskSuspend(connectGPRSHandle);
	}
}

static void httpProc(void * pvParameters)
{
	uint32_t _weight;
	char gen_buff[64];
	for(;;)
	{
	    if( httpQueue != 0 )
	    {
	        if( xQueueReceive( httpQueue, &( _weight ), ( portTickType ) 10 ) )
	        {
	        	sprintf(gen_buff, "{\"w_val\":%d}",_weight);
				#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("msg received by http\r\n");
						xSemaphoreGive(debugSema);
					}
				#endif

				if(gsm_http_put("lit-taiga-2854.herokuapp.com","/weight/1", gen_buff))
				{
					http_read_data(&modem);

					if(gsm_tcp_disconnect())
					{
						#ifdef __DEBUG_MESSAGES__
							if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
							{
								debug_out("tcp disconnect success\r\n");
								xSemaphoreGive(debugSema);
							}
						#endif					
					}
					else
					{
						#ifdef __DEBUG_MESSAGES__
						if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
						{
							debug_out("tcp disconnect failed\r\n");
							xSemaphoreGive(debugSema);
						}
						#endif
					}
				}
				else
				{
					#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("http failed\r\n");
						xSemaphoreGive(debugSema);
					}
					#endif					
				}
			}
		}
	}
}


static void updateModemStatus(void * pvParameters)
{
	for(;;)
	{

	}	
}

static void displayProcess(void * pvParameters)
{
	uint8_t count = 0;
	char buff[20];
	uint32_t __weight;
	for(;;)
	{
	/*	if(count == 0)
		{
			lcd_write_instruction_4d(0x80);
			lcd_print("   WEIGHT    ");
			sprintf(buff, "%d g", weight_val);
			lcd_write_instruction_4d(0xC0);
			lcd_print(buff);
			lcd_print("                     ");
			count++;
		}*/

		if(count == 0)
		{
			lcd_write_instruction_4d(0x80);
			lcd_print("  IP ADDR    ");
			lcd_write_instruction_4d(0xC0);
			lcd_print(modem.ip_addr);
			lcd_print("                     ");
			count++;
		}

		else if(count == 1)
		{
			lcd_write_instruction_4d(0x80);
			lcd_print("     GPRS    ");
			lcd_write_instruction_4d(0xC0);
			lcd_print(modem.tcpstatus);
			lcd_print("                     ");
			count = 0;
		}

		if( xQueueReceive( lcdQueue, &( __weight ), ( portTickType ) 10 ) )
		{
			lcd_write_instruction_4d(0x80);
			lcd_print("   WEIGHT    ");
			sprintf(buff, "%d g", __weight);
			lcd_write_instruction_4d(0xC0);
			lcd_print(buff);
			lcd_print("                     ");
		}

		vTaskDelay(1000);
	}	
}

static void sendSms(void * pvParameters)
{
	uint32_t _weight;
	char buff[32];
	for(;;)
	{
		if(smsQueue != 0 )
	    {
	        if( xQueueReceive( smsQueue, &( _weight ), ( portTickType ) 10 ) )
	        {
				#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("msg received by sms task\r\n");
						xSemaphoreGive(debugSema);
					}
				#endif

				sprintf(buff, "alert: weight is %d grams", _weight);

				if(gsm_send_sms(__NUM, buff))
				{
					#ifdef __DEBUG_MESSAGES__
						if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
						{
							debug_out("sms sent\r\n");
							xSemaphoreGive(debugSema);
						}
					#endif					
				}

	        }
	    }
	}	
}

static void measureWeight(void * pvParameters)
{
	uint8_t len;

	uint32_t last_read = 0;

	uint32_t _weight;

	#ifdef __DEBUG_MESSAGES__
		if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
		{
			debug_out("Measure weight task started\r\n");
			xSemaphoreGive(debugSema);
		}
	#endif

	for(;;)
	{
		len = uart0_readline();
		char 		__next;
		uint32_t 	__upper 	= strtol(uart0_fifo.line, &__next , 10);
		char *		_next 		= index(uart0_fifo.line, '.') + 1;
		uint32_t 	__lower 	= strtol(_next, &__next , 10);
		uint32_t 	val 		= (__upper * 1000) + __lower;

		int32_t temp;

		if(last_read != val)
		{
			vTaskDelay(1000);
			if(last_read > val)
				temp = last_read - val;
			else if(val > last_read)
				temp = val - last_read;

			_weight = val;
			last_read = val;

			if( xQueueSend( lcdQueue, ( void * ) &_weight, ( portTickType ) 10 ) );

			// /*if(temp > WEIGHT_THRESHOLD)
			// {

	  //       	#ifdef __DEBUG_MESSAGES__
		 //            if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			// 		{
			// 			debug_out("threshold crossed\r\n");
			// 			xSemaphoreGive(debugSema);
			// 		}
			// 	#endif
			// 	/* send to sms task */
			// 	if( xQueueSend( smsQueue, ( void * ) &_weight, ( portTickType ) 10 ) != pdPASS )
		 //        {
		 //        	#ifdef __DEBUG_MESSAGES__
			//             if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			// 			{
			// 				debug_out("failed to send to sms queue\r\n");
			// 				xSemaphoreGive(debugSema);
			// 			}
			// 		#endif
		 //        }
			// }*/

				if( xQueueSend( httpQueue, ( void * ) &_weight, ( portTickType ) 10 ) != pdPASS )
		        {
		        	#ifdef __DEBUG_MESSAGES__
			            if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
						{
							debug_out("failed to send to http queue\r\n");
							xSemaphoreGive(debugSema);
						}
					#endif
		        }			

			
/*			else
			{
	            if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("sent to queue\r\n");
					xSemaphoreGive(debugSema);
				}					
			}*/

			/* send to http task */
			
		}
		vTaskDelay(1000);
	}	
}



