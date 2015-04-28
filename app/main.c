
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

/* Setup Hardware Function */
//////////////////////////////////////////////////////
void 	prvSetupHardware(void);
//////////////////////////////////////////////////////

/* Tasks */
//////////////////////////////////////////////////////
static void systemBoot(void * pvParameters);
static void connectGPRS(void * pvParameters);
static void updateModemStatus(void * pvParameters);
static void displayProcess(void * pvParameters);
static void scanCard(void * pvParameters);
static void scankeyPad(void * pvParameters);
static void httpProc(void * pvParameters);
//////////////////////////////////////////////////////

/* Semaphores */
//////////////////////////////////////////////////////
xSemaphoreHandle modemSema;
xSemaphoreHandle displaySema;
xSemaphoreHandle scanCardSema;
xSemaphoreHandle debugSema;
//////////////////////////////////////////////////////

/* Task Handles */
//////////////////////////////////////////////////////
xTaskHandle systemBootHandle;
xTaskHandle connectGPRSHandle;
xTaskHandle updateModemStatusHandle;
xTaskHandle displayProcessHandle;
xTaskHandle scanCardHandle;
xTaskHandle httpTaskHandle;
xTaskHandle keypadTaskHandle;
//////////////////////////////////////////////////////

/* Queues */
//////////////////////////////////////////////////////
xQueueHandle httpQueue;
xQueueHandle lcdQueue;
xQueueHandle keypadQueue;
//////////////////////////////////////////////////////

/* APN/OPR List size */
#define APN_OPR_LIST_LEN 8

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

/*	while(1)
	{
		char c = read_keypad();
		if(c != 0)
			debug_putc(c);
	}*/

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

	lcd_write_instruction_4d(0x80);
	lcd_print("system started");	

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
		//////////////////////////////////////////////////////
		modemSema 			= xSemaphoreCreateMutex();
		displaySema			= xSemaphoreCreateMutex();
		scanCardSema		= xSemaphoreCreateMutex();
		debugSema			= xSemaphoreCreateMutex();
		//////////////////////////////////////////////////////

		if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
		{
			lcd_write_instruction_4d(0xC0);
			lcd_print("semaphore create");
			xSemaphoreGive(displaySema);
		}

		/* ping modem for 8 times */
		//////////////////////////////////////////////////////
		for(uint8_t i = 0;i < 8;i++)
		{
			if(gsm_ping_modem())
			{
				debug_out("ping: ");
				debug_putc(i + 48); 
				debug_out("\r\n");
			}	
		}
		//////////////////////////////////////////////////////

		/* Get operator name */
		__response = gsm_get_operator_name(&modem);

		/* convert to lower for comparison */
		strtolower(modem.operator_name);

		/* find out apn */
		//////////////////////////////////////////////////////
		for(uint8_t i = 0;i < APN_OPR_LIST_LEN;i++)
		{
			if(strstr(modem.operator_name, oprList[i]))
			{
				strcpy(modem.setapn, apnList[i]);
				break;
			}
		}
		//////////////////////////////////////////////////////

		#ifdef __DEBUG_MESSAGES__
			debug_out("apn search completed\r\n");
			debug_out("access point name for operator ");
			debug_out(modem.operator_name);
			debug_out(" is ");
			debug_out(modem.setapn);
			debug_out("\r\n");
			debug_out("reading apn from modem\r\n");
		#endif

		/* read apn from modem */
		__response = gsm_get_accesspoint(&modem);

		#ifdef __DEBUG_MESSAGES__
			debug_out("read apn completed\r\n");
			debug_out("access point read from modem: ");
			debug_out(modem.getapn);
			debug_out("\r\n");
		#endif

		/* Check with predefined APN */
		if(strstr(modem.getapn, modem.setapn) == NULL)
		{
			/* If both are not matched set access point */
			#ifdef __DEBUG_MESSAGES__
				debug_out("access point read from modem is different from actual\r\n");
				debug_out("So setting actual accesspoint name\r\n");
			#endif

			if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
			{
				lcd_write_instruction_4d(0x80);
				lcd_print("apn setting");
				lcd_print("             ");
				xSemaphoreGive(displaySema);
			}

			/* set access point */
			__response = gsm_set_accesspoint(&modem);

			if(__response)
			{
				#ifdef __DEBUG_MESSAGES__
					debug_out("set accesspoint name success\r\n");
					debug_out("reading apn again from modem\r\n");
				#endif		

				/* read access point */
				__response = gsm_get_accesspoint(&modem);

				#ifdef __DEBUG_MESSAGES__
					debug_out("read apn completed\r\n");
					debug_out("apn name read: ");
					debug_out(modem.getapn);
					debug_out(", actual: ");
					debug_out(modem.setapn);
					debug_out("\r\n");
				#endif					
			}
		}

		/* else modem has valid apn name */
		else
		{
			#ifdef __DEBUG_MESSAGES__
				debug_out("modem has valid apn no need to set\r\n");
			#endif			
		}

		/* create queues */
		//////////////////////////////////////////////////////
		httpQueue  	= xQueueCreate(10, 6);
		lcdQueue   	= xQueueCreate(10, sizeof(uint8_t     ));
		keypadQueue = xQueueCreate(10, sizeof(char));
		//////////////////////////////////////////////////////

		if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
		{
			lcd_write_instruction_4d(0x80);
			lcd_print("queue create");
			lcd_print("             ");
			xSemaphoreGive(displaySema);
		}

		#ifdef __DEBUG_MESSAGES__
			debug_out("queues are created, now creating the\r\ntasks after that this task will delete\r\n");
		#endif				

		/* Create GPRS Task */
		//////////////////////////////////////////////////////
		xTaskCreate(	connectGPRS,
						(signed portCHAR *)"gprs",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&connectGPRSHandle);

		/* Create GPRS Task */
		xTaskCreate(	httpProc,
						(signed portCHAR *)"http",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&httpTaskHandle);

		/* Create GPRS Task */
		xTaskCreate(	scanCard,
						(signed portCHAR *)"scancard",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&scanCardHandle);

		/* Create GPRS Task */
		xTaskCreate(	displayProcess,
						(signed portCHAR *)"display",
						configMINIMAL_STACK_SIZE,
						NULL,
						tskIDLE_PRIORITY,
						&displayProcessHandle);
		//////////////////////////////////////////////////////

		if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
		{
			lcd_write_instruction_4d(0x80);
			lcd_print("tasks created");
			lcd_print("             ");
			xSemaphoreGive(displaySema);
		}

		/* delete the boot task */
		vTaskDelete(systemBootHandle);
	}
}

/** @brief 
 *  @param 
 *  @return 
 */
static void connectGPRS(void * pvParameters)
{
	int8_t __response;

	// #ifdef __DEBUG_MESSAGES__
	// 	debug_out("GPRS task started\r\n");
	// #endif

	for(;;)
	{
		/* Check IP Status and check whether shutdown is required */
		#ifdef __DEBUG_MESSAGES__
			debug_out("reading tcp status from modem\r\n");
		#endif

		/* Get TCP Status */
		__response = gsm_get_tcpstatus(&modem);

		/* lower case the state for comparison */
		strtolower(modem.tcpstatus);

		#ifdef __DEBUG_MESSAGES__
			debug_out("reading tcp status completed\r\n");
			debug_out("status: ");
			debug_out(modem.tcpstatus);
			debug_out("\r\n");
		#endif

		/* check tcpstatus and do what action is required to bring wireless up */
		if(strstr(modem.tcpstatus, "ip start") || strstr(modem.tcpstatus, "ip initial"))
		{
			#ifdef __DEBUG_MESSAGES__
				debug_out("ip start state bringing wireless up\r\n");
			#endif				
			if(gsm_start_gprs())
			{
				#ifdef __DEBUG_MESSAGES__
					debug_out("GPRS start success\r\n");
					debug_out("reading ip address\r\n");
				#endif	

				/* read ip address from modem */
				if(gsm_get_ip_address(&modem))
				{
					#ifdef __DEBUG_MESSAGES__
						debug_out("Read IP address success\r\n");
						debug_out(modem.ip_addr);
					#endif	
				}
			}
		}

		/* Not Implemented */
		else if(strstr(modem.tcpstatus, "ip config"))
		{
			#ifdef __DEBUG_MESSAGES__
				debug_out("ip configure\r\n");
			#endif				
		}

		/* If gprs is already activated just read ip address from the modem */
		else if(strstr(modem.tcpstatus, "ip gprsact") || strstr(modem.tcpstatus, "ip status"))
		{
			#ifdef __DEBUG_MESSAGES__
					debug_out("gprs act\r\n");
			#endif	

			#ifdef __DEBUG_MESSAGES__
				debug_out("reading ip address\r\n");
			#endif	

			/* read ip address from modem */
			if(gsm_get_ip_address(&modem))
			{
				#ifdef __DEBUG_MESSAGES__
					debug_out("Read IP address success\r\n");
					debug_out(modem.ip_addr);
				#endif	
			}
		}

		/* Not Implemented */
		else if(strstr(modem.tcpstatus, "tcp connecting"))
		{
			#ifdef __DEBUG_MESSAGES__
				debug_out("connecting\r\n");
			#endif				
		}

		/* If it already connected disconnect 1st and then read ip address*/
		else if(strstr(modem.tcpstatus, "connect ok"))
		{
			#ifdef __DEBUG_MESSAGES__
				debug_out("connect ok\r\n");
			#endif	

			/* disconnect */
			if(gsm_tcp_disconnect())
			{
				#ifdef __DEBUG_MESSAGES__
					debug_out("disconnect OK\r\n");
				#endif	

				/* read ip address from modem */
				if(gsm_get_ip_address(&modem))
				{
					#ifdef __DEBUG_MESSAGES__
						debug_out("Read IP address success\r\n");
						debug_out(modem.ip_addr);
					#endif	
				}
			}			
		}

		/* TODO: Not tested */
		else if(strstr(modem.tcpstatus, "tcp closing"))
		{
			#ifdef __DEBUG_MESSAGES__
				debug_out("tcp closing\r\n");
			#endif	

			/* read ip address from modem */
			if(gsm_get_ip_address(&modem))
			{
				#ifdef __DEBUG_MESSAGES__
					debug_out("Read IP address success\r\n");
					debug_out(modem.ip_addr);
				#endif	
			}			
		}

		/* TODO: Not tested */ 
		else if(strstr(modem.tcpstatus, "tcp closed"))
		{
			#ifdef __DEBUG_MESSAGES__
				debug_out("tcp closed\r\n");
			#endif				

			/* read ip address from modem */
			if(gsm_get_ip_address(&modem))
			{
				#ifdef __DEBUG_MESSAGES__
					debug_out("Read IP address success\r\n");
					debug_out(modem.ip_addr);
				#endif	
			}	
		}

		/* FIXME: Dont know what to do? If deactivated :( */
		else if(strstr(modem.tcpstatus, "pdp deact"))
		{
			// FIXME: What to do?
			#ifdef __DEBUG_MESSAGES__
				debug_out("pdp deactivated shutdown is required\r\n");
			#endif			
		}
		else
		{
			/* dont know what status it is */
			/* read ip address from modem */
			if(gsm_get_ip_address(&modem))
			{
				#ifdef __DEBUG_MESSAGES__
					debug_out("Read IP address success\r\n");
					debug_out(modem.ip_addr);
				#endif	
			}
		}

		#ifdef __DEBUG_MESSAGES__
			debug_out("gprs task completed, suspending now\r\n");
		#endif	

		if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
		{
			lcd_write_instruction_4d(0x01);
			lcd_write_instruction_4d(0x80);
			lcd_print("GPRS ENABLE");
			lcd_print("             ");
			xSemaphoreGive(displaySema);
		}

		if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
		{
			lcd_write_instruction_4d(0x01);
			lcd_write_instruction_4d(0x80);
			lcd_print("    WELCOME           ");
			xSemaphoreGive(displaySema);
		}

		/* suspend the task */
		vTaskSuspend(connectGPRSHandle);
	}
}

static void httpProc(void * pvParameters)
{
	uint8_t i;
	char rfid[10];
	char * path;
	path = (char *) malloc(64);
	char name[20];
	char amt[10];

	char to[12];

	char key[10];
	char option;



	for(;;)
	{
		back:
		/* check whether queue is created successfully */
	    if( httpQueue != 0 )
	    {
	    	/* Wait for queue */
	        if( xQueueReceive( httpQueue, ( rfid ), ( portTickType ) 10 ) )
	        {
	        	/*bzero(path, 60);
	        	strcpy(path, BASE);*/
		        if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
				{
					lcd_write_instruction_4d(0x80);
					lcd_print("Card read  ");
					lcd_print("             ");
					lcd_write_instruction_4d(0xC0);
					lcd_print("connecting............");
					xSemaphoreGive(displaySema);
				}

	        	/* send card number to server and get validation result */
	        	bzero(path, 60);
	        	strcpy(path, BASE);
	        	memcpy(&path[strlen(path)], rfid, 6);
	        	path[strlen(BASE) + 6] = NULL;
	        	
				if(gsm_http_get(URL, path))
	        	{
		        	if( xSemaphoreTake( debugSema, ( portTickType ) 50 ) == pdTRUE )
					{
						debug_out("HTTP OK\r\n");
						/*debug_out(path);
						debug_out("\r\n");*/
						xSemaphoreGive(debugSema);
					}

					if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
					{
						lcd_write_instruction_4d(0x80);
						lcd_print("connected      ");
						lcd_print("             ");
						lcd_write_instruction_4d(0xC0);
						lcd_print("                       ");
						xSemaphoreGive(displaySema);
					}

					http_read_data(&modem);

					/* {"id":18,"created_at":"2015-04-27T12:55:06.337Z","updated_at":"2015-04-28T02:45:33.063Z",
					"amount":1000,"card":"1A2643","name":"person2"} */

					if(strstr(modem.httpdata, "notfound"))
					{
						if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
						{
							lcd_write_instruction_4d(0x80);
							lcd_print("ERROR               ");
							lcd_print("             ");
							lcd_write_instruction_4d(0xC0);
							lcd_print("Invalid card       ");
							xSemaphoreGive(displaySema);
						}	
						modem_flush_rx();
						goto back;				
					}

					char * _name = strstr(modem.httpdata, "name");
					_name += sizeof("name\":");
					char * c = index(_name, '"');

					uint8_t __size = strlen(_name) - strlen(c);

					memcpy(name, _name, __size);
					name[__size] = '\0';

					char * _amt = strstr(modem.httpdata, "amount");
					_amt += sizeof("amount:");
					c = index(_amt, ',');

					__size = strlen(_amt) - strlen(c);

					memcpy(amt, _amt, __size);
					amt[__size] = '\0';

					/* Parse data here */
					if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
					{
						lcd_write_instruction_4d(0x80);
						lcd_print("WELCOME              ");
						lcd_print("             ");
						lcd_write_instruction_4d(0xC0);
						lcd_print(name);
						xSemaphoreGive(displaySema);
					}

					debug_out("DATA\r\n");
					debug_out(modem.httpdata);
					debug_out("\r\n");

					vTaskDelay(100);

					if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
					{
						lcd_write_instruction_4d(0x80);
						lcd_print("OPTIONS           ");
						lcd_print("             ");
						lcd_write_instruction_4d(0xC0);
						lcd_print("1.BQ, 2.PB, 3.TR  ");
						xSemaphoreGive(displaySema);
					}	

					do
					{
						option = read_keypad();
					}
					while(option == 0);

					debug_putc(option);

					if(option == '1')
					{
						if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
						{
							lcd_write_instruction_4d(0x80);
							lcd_print("BALANCE IS       ");
							lcd_print("             ");
							lcd_write_instruction_4d(0xC0);
							lcd_print(amt);
							lcd_print(" INR              ");
							xSemaphoreGive(displaySema);
						}	
					}

					else if(option == '2')
					{
						if(gsm_tcp_disconnect())
						{
							if( xSemaphoreTake( debugSema, ( portTickType ) 50 ) == pdTRUE )
							{
								debug_out("DISCONNECT OK\r\n");
								xSemaphoreGive(debugSema);
							}	
						}
						if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
						{
							lcd_write_instruction_4d(0x01);
							lcd_write_instruction_4d(0x80);
							lcd_print("ENTER BILLER");
							lcd_print("             ");
							lcd_write_instruction_4d(0xC0);
							xSemaphoreGive(displaySema);
						}	
						char temp[2];
						do
						{
							temp[0] = read_keypad();
							if(temp[0] != 0)
								lcd_write_character_4d(temp[0]);
						}
						while(temp[0] == 0);
						temp[1] = NULL;
						

						strcat(path, "/");
						strcat(path, temp);
						strcat(path, "/");

						if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
						{
							lcd_write_instruction_4d(0x01);
							lcd_write_instruction_4d(0x80);
							lcd_print("ENTER AMOUNT");
							lcd_print("             ");
							lcd_write_instruction_4d(0xC0);
							xSemaphoreGive(displaySema);
						}	

						char __temp;
						uint8_t __i = 0;
						do
						{
							__temp = read_keypad();
							if(__temp != 0 && __temp != '*')
							{
								amt[__i++] = __temp;
								lcd_write_character_4d(__temp);
							}
						}
						while(__temp != '*');
						amt[__i] = NULL;

						strcat(path, amt);
						debug_out(path);

						if(gsm_http_get(URL, path))
						{
							if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
							{
								lcd_write_instruction_4d(0x80);
								lcd_print("connected      ");
								lcd_print("             ");
								lcd_write_instruction_4d(0xC0);
								lcd_print("                       ");
								xSemaphoreGive(displaySema);
							}
							http_read_data(&modem);		

							if(strstr(modem.httpdata, "success"))
							{
								if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
								{
									lcd_write_instruction_4d(0x80);
									lcd_print("TRANSACTION       ");
									lcd_print("             ");
									lcd_write_instruction_4d(0xC0);
									lcd_print("SUCCESS                ");
									xSemaphoreGive(displaySema);
								}								
							}	
							else if(strstr(modem.httpdata, "insuffi"))
							{
								if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
								{
									lcd_write_instruction_4d(0x80);
									lcd_print("TRANSACTION       ");
									lcd_print("             ");
									lcd_write_instruction_4d(0xC0);
									lcd_print("BAL IS LOW       ");
									xSemaphoreGive(displaySema);
								}	
							}
							else
							{
								if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
								{
									lcd_write_instruction_4d(0x80);
									lcd_print("TRANSACTION       ");
									lcd_print("             ");
									lcd_write_instruction_4d(0xC0);
									lcd_print("ERROR             ");
									xSemaphoreGive(displaySema);
								}									
							}
						}
					}

					else if(option == '3')
					{
						if(gsm_tcp_disconnect())
						{
							if( xSemaphoreTake( debugSema, ( portTickType ) 50 ) == pdTRUE )
							{
								debug_out("DISCONNECT OK\r\n");
								xSemaphoreGive(debugSema);
							}	
						}
						if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
						{
							lcd_write_instruction_4d(0x01);
							lcd_write_instruction_4d(0x80);
							lcd_print("ENTER DEST");
							lcd_print("             ");
							lcd_write_instruction_4d(0xC0);
							xSemaphoreGive(displaySema);
						}	
						char __temp = 0;
						uint8_t __i = 0;
						do
						{
							__temp = read_keypad();
							if(__temp != 0 && __temp != '*')
							{
								to[__i++] = __temp;
								lcd_write_character_4d(__temp);
							}
						}
						while(__temp != '*');
						to[__i] = NULL;

						debug_out(path);
						

						strcat(path, "/");
						strcat(path, to);
						strcat(path, "/");

						debug_out(path);

						if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
						{
							lcd_write_instruction_4d(0x01);
							lcd_write_instruction_4d(0x80);
							lcd_print("ENTER AMOUNT");
							lcd_print("             ");
							lcd_write_instruction_4d(0xC0);
							xSemaphoreGive(displaySema);
						}	

						 __temp = 0;
						 __i = 0;
						do
						{
							__temp = read_keypad();
							if(__temp != 0 && __temp != '*')
							{
								amt[__i++] = __temp;
								lcd_write_character_4d(__temp);
							}
						}
						while(__temp != '*');
						amt[__i] = NULL;

						strcat(path, amt);
						debug_out(path);

						if(gsm_http_get(URL, path))
						{
							if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
							{
								lcd_write_instruction_4d(0x80);
								lcd_print("connected      ");
								lcd_print("             ");
								lcd_write_instruction_4d(0xC0);
								lcd_print("                       ");
								xSemaphoreGive(displaySema);
							}
							http_read_data(&modem);		

							if(strstr(modem.httpdata, "success"))
							{
								if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
								{
									lcd_write_instruction_4d(0x80);
									lcd_print("TRANSACTION       ");
									lcd_print("             ");
									lcd_write_instruction_4d(0xC0);
									lcd_print("SUCCESS                ");
									xSemaphoreGive(displaySema);
								}								
							}	
							else if(strstr(modem.httpdata, "insuffi"))
							{
								if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
								{
									lcd_write_instruction_4d(0x80);
									lcd_print("TRANSACTION       ");
									lcd_print("             ");
									lcd_write_instruction_4d(0xC0);
									lcd_print("BAL IS LOW       ");
									xSemaphoreGive(displaySema);
								}	
							}
							else
							{
								if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
								{
									lcd_write_instruction_4d(0x80);
									lcd_print("TRANSACTION       ");
									lcd_print("             ");
									lcd_write_instruction_4d(0xC0);
									lcd_print("ERROR             ");
									xSemaphoreGive(displaySema);
								}									
							}
						}
					}


					/*if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
					{
						lcd_write_instruction_4d(0x80);
						lcd_print("OPTIONS           ");
						lcd_print("             ");
						lcd_write_instruction_4d(0xC0);
						lcd_print("1.BQ, 2.PB, 3.TR  ");
						xSemaphoreGive(displaySema);
					}*/
					

						
					if(gsm_tcp_disconnect())
					{
						if( xSemaphoreTake( debugSema, ( portTickType ) 50 ) == pdTRUE )
						{
							debug_out("DISCONNECT OK\r\n");
							xSemaphoreGive(debugSema);
						}	
					}
					else
					{
						if( xSemaphoreTake( debugSema, ( portTickType ) 50 ) == pdTRUE )
						{
							debug_out("DISCONNECT OK\r\n");
							xSemaphoreGive(debugSema);
						}	
					}
				}

				else
	        	{
					if( xSemaphoreTake( debugSema, ( portTickType ) 50 ) == pdTRUE )
					{
						debug_out("HTTP FAIL\r\n");
						xSemaphoreGive(debugSema);
					}
	        	}

	        	vTaskDelay(500);

				if( xSemaphoreTake( displaySema, ( portTickType ) 10 ) == pdTRUE )
				{
					lcd_write_instruction_4d(0x01);
					lcd_write_instruction_4d(0x80);
					lcd_print("    WELCOME           ");
					xSemaphoreGive(displaySema);
				}	
		  
	        	
	        	/* wait for modem semaphore */
	        	// if(xSemaphoreTake(modemSema, (portTickType) 10) == pdTRUE)
	        	// {
	        	// 	/* got resource */

	        	// 	 send card number to server and get validation result 
	        		

	        	// }
	        	// else
	        	// {
	        	// 	/* resource busy */
	        	// }
			}
			else
				modem_flush_rx();
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
	char card_no[15];
	char rfid[15];
	uint8_t i = 0;
	for(;;)
	{
		while(i != 12)
		{
			if(uart0_fifo.num_bytes > 0)
				card_no[i++] = uart0_getc();
		}

		char * c = &card_no[4];
		memcpy(rfid, c, 6);
		//rfid[6] = NULL;

		i = 0;
		uart0_flushrx();

		/* send queue to http process */

		xQueueSend( httpQueue, ( void * ) rfid, ( portTickType ) 10 );
	}	
}



