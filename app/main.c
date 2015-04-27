
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

/* streams (stdout, stdin, stderr) */
FILE * stdin;
FILE * stdout;
FILE * stderr;

/* Define how many number of operator or apn we have (opr and apn array length should be same) */
#define APN_OPR_LIST_LEN 8

/* Setup Hardware Function */
void 	prvSetupHardware(void);

/* Tasks */
static void systemBoot(void * pvParameters);
static void connectGPRS(void * pvParameters);
static void updateModemStatus(void * pvParameters);
static void displayProcess(void * pvParameters);
static void scanCard(void * pvParameters);
static void httpProc(void * pvParameters);

/* Semaphores */
xSemaphoreHandle modemSema;
xSemaphoreHandle displaySema;
xSemaphoreHandle scanCardSema;

#ifdef __DEBUG_MESSAGES__
	xSemaphoreHandle debugSema;
#endif

/* Task Handles */
xTaskHandle systemBootHandle;
xTaskHandle connectGPRSHandle;
xTaskHandle updateModemStatusHandle;
xTaskHandle displayProcessHandle;
xTaskHandle scanCardHandle;
xTaskHandle httpTaskHandle;

/* Queues */
xQueueHandle httpQueue;
xQueueHandle lcdQueue;

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
		scanCardSema		= xSemaphoreCreateMutex();
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

		/* create queue */
		httpQueue = xQueueCreate(10, sizeof(uint8_t));

		xSemaphoreGive(scanCardSema);
		xSemaphoreGive(modemSema);

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
		xTaskCreate(	scanCard,
						(signed portCHAR *)"rfid",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&scanCardHandle);

		/* Create GPRS Task */
		xTaskCreate(	displayProcess,
						(signed portCHAR *)"rfid",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&displayProcessHandle);

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
		else if(strstr(modem.tcpstatus, "ip gprsact") || strstr(modem.tcpstatus, "ip status"))
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
		else if(strstr(modem.tcpstatus, "tcp closed"))
		{
			#ifdef __DEBUG_MESSAGES__
				if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
				{
					debug_out("tcp closed\r\n");
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



		if( xQueueSend( httpQueue, ( void * ) &__flag, ( portTickType ) 10 ) != pdPASS )
        {
            if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
			{
				debug_out("failed to send to queue\r\n");
				xSemaphoreGive(debugSema);
			}
        }

		/* suspend the task */
		vTaskSuspend(connectGPRSHandle);
	}
}

static void httpProc(void * pvParameters)
{
	uint8_t __value;
	uint8_t __flag = 0;
	for(;;)
	{
	    if( httpQueue != 0 )
	    {
	        if( xQueueReceive( httpQueue, &( __value ), ( portTickType ) 10 ) )
	        {
				#ifdef __DEBUG_MESSAGES__
					if( xSemaphoreTake( debugSema, ( portTickType ) 10 ) == pdTRUE )
					{
						debug_out("msg receied started http\r\n");
						xSemaphoreGive(debugSema);
					}
				#endif	

				__flag = 1;
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
	for(;;)
	{

	}	
}

static void scanCard(void * pvParameters)
{
	uint8_t len;
	uint8_t i = 0;
	uint8_t scanCompleted = 0;
	char card_no[15];

	char rfid[15];

	#ifdef __DEBUG_MESSAGES__
			if( xSemaphoreTake( debugSema, ( portTickType ) 500 ) == pdTRUE )
			{
				debug_out("Scanning task started\r\n");
				xSemaphoreGive(debugSema);
			}
	#endif
	for(;;)
	{
		if(uart0_fifo.num_bytes > 0)
		{
			char c = uart0_getc();
			card_no[i++] = c;
		}

		if(i == 12)
		{
			memcpy(rfid, card_no, 10);
			rfid[10] = '\0';
			i = 0;

			/* send signal to http process */
			if( xSemaphoreTake( debugSema, ( portTickType ) 50 ) == pdTRUE )
			{
				debug_out("no: ");
				debug_out(rfid);
				debug_out("\r\n");
				xSemaphoreGive(debugSema);
			}	
		}	
	}	
}



