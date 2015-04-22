
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
#include "jsmn.h"

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

/* protos */
void prvSetupHardware(void);
void debug_out(char *);
void modem_out(char *);
uint8_t process_response(char * ptr, uint16_t len);

/* process */
static void sysboot(void * pvParameters);
static void monModem(void * pvParameters);
static void establishGPRS(void * pvParameters);
static void monZigbee(void * pvParameters);
static void display(void * pvParameters);

/* semaphores */
xSemaphoreHandle modemUARTSemaphore;
xSemaphoreHandle lcdSemaphore;
xSemaphoreHandle zigbeeSemaphore;

/* Task Handles */
xTaskHandle monTaskHandle, bootTaskHandle, GPRStaskHandle, monZigbeeHandle, displayHandle;

/* buffer for debug sprintf */
char debug_buff[64];

typedef struct 
{
	uint8_t ref;
	uint8_t d_state;
	uint16_t s_val;				
}dbType_t;

dbType_t db0, db1, db2;

int main(void)
{
	/* Setup the Hardware */
	prvSetupHardware();

	xTaskCreate(sysboot,
			(signed portCHAR *)"sysboot",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY,
			&bootTaskHandle);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Never reach here */
	return 0;
}

/* setup hardware */
void prvSetupHardware(void)
{
	SystemInit();
	SystemCoreClockUpdate();
	uart0_init(9600);							// Debug port

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("debug port is up\r\n");
	#endif

	uart1_init(9600);							// zigbee port
	uart3_init(9600);							// modem port

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("modem and zigbee ports are up\r\n");
	#endif

	lcd_init();									// init lcd

	#if APPLICATION_LOG_LEVEL == 1
		debug_out("lcd init success\r\n");
	#endif

	#if LCD_INIT_PRINT == 1
		lcd_write_instruction_4d(0x01);
		lcd_write_instruction_4d(0x80);
		lcd_print("-hardware init-");
		lcd_write_instruction_4d(0xC0);
		lcd_print("--- success ---");
	#endif

	gsm_allocate_mem();

	/*db.created_at 	= (char *) malloc(70);
	db.updated_at 	= (char *) malloc(70);
	db.id 			= (char *) malloc(5);
	db.ref 			= (char *) malloc(5);
	db.d_state 		= (char *) malloc(5);
	db.s_val 		= (char *) malloc(10);*/

	LPC_GPIO1->FIODIR |= (1 << 19);
	LPC_GPIO1->FIODIR |= (1 << 20);
	LPC_GPIO1->FIODIR |= (1 << 21);

	LPC_GPIO1->FIOSET |= (1 << 19);
	LPC_GPIO1->FIOSET |= (1 << 20);
	LPC_GPIO1->FIOSET |= (1 << 21);
}


/* processA */
static void sysboot(void * pvParameters)
{
	for(;;)
	{
		uint16_t len;
		modemUARTSemaphore 	= xSemaphoreCreateMutex();
		lcdSemaphore		= xSemaphoreCreateMutex();
		zigbeeSemaphore		= xSemaphoreCreateMutex();

		#if APPLICATION_LOG_LEVEL == 1
				debug_out("system botting\r\n");
				debug_out("semaphores created\r\n");
		#endif

		if(modemUARTSemaphore == NULL){
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("modemUARTSemaphore create failed\r\n");
			#endif
		}
		else{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("modemUARTSemaphore create success\r\n");
			#endif
		}

		if(lcdSemaphore == NULL){
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("lcdSemaphore create failed\r\n");
			#endif
		}
		else{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("lcdSemaphore create success\r\n");
			#endif
		}	


		if(zigbeeSemaphore == NULL){
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("zigbeeSemaphore create failed\r\n");
			#endif
		}
		else{
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("zigbeeSemaphore create success\r\n");
			#endif
		}		
	

		#if MALLOC_TEST == 1
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("malloc test started\r\n");
			#endif
			char * c;
			#if APPLICATION_LOG_LEVEL == 1
				debug_out("allocating 1024 bytes\r\n");
			#endif
			c = (char *) malloc(1024);
			if(c != NULL)
			{
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("malloc ok\r\n");
				#endif
			}
			else
			{
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("malloc fail\r\n");
				#endif
			}
			#if APPLICATION_LOG_LEVEL == 1 
				debug_out("free 1024 bytes\r\n");
				debug_out("free 1024 bytes success\r\n");
			#endif
			free(c);
		#endif


		#if APPLICATION_LOG_LEVEL == 1
			debug_out("creating tasks\r\n");
		#endif

		if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
			// monitor modem
			xTaskCreate(monModem,
					(signed portCHAR *)"monModem",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&monTaskHandle);

			// create task and resume until OK is received from above task
			xTaskCreate(establishGPRS,
					(signed portCHAR *)"GPRS",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&GPRStaskHandle);	

			xTaskCreate(display,
					(signed portCHAR *)"display",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&displayHandle);	

			xTaskCreate(monZigbee,
					(signed portCHAR *)"zigbee",
					configMINIMAL_STACK_SIZE,
					NULL,
					tskIDLE_PRIORITY,
					&monZigbeeHandle);	

			if(monTaskHandle != NULL)
			{
				//vTaskSuspend(monModem);
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("monModem task create success\r\n");
				#endif			
			}
			else
			{
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("monModem task create error\r\n");
				#endif		
			}

			if(GPRStaskHandle != NULL)
			{
				//vTaskSuspend(GPRStaskHandle);
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("establishGPRS task create success\r\n");
				#endif			
			}
			else
			{
				#if APPLICATION_LOG_LEVEL == 1
					debug_out("establishGPRS task create error\r\n");
				#endif		
			}

			#if APPLICATION_LOG_LEVEL == 1
				debug_out("returning semaphore\r\n");
			#endif

			#if APPLICATION_LOG_LEVEL == 1
				debug_out("deleting boot task\r\n");
			#endif	

			uint8_t state = gsm_get_opr_name();
			if(state)
			{
				debug_out(modem.opr);
				debug_out("\r\n");
			}

			// query apn
			state = gsm_get_apn();
			if(state)
			{
				debug_out(modem.apn);
				debug_out("\r\n");
			}

			uint8_t trials = 0;

			back2:
			// update apn
			if(gsm_set_apn("TATA.DOCOMO.INTERNET"))
			{
				debug_out("apn ok\r\n");
			}
			else
			{
				trials++;
				if(trials < 10)
				{
					vTaskDelay(100);
					goto back2;
				}
				debug_out("apn fail\r\n");
			}

			// query apn
			state = gsm_get_apn();
			if(state)
			{
				debug_out(modem.apn);
				debug_out("\r\n");
			}

			state = gsm_bring_wireless_up();
			if(state)
			{
				debug_out("ciicr success\r\n");
			}
			else
			{
				debug_out("ciicr fail\r\n");
			}

			state = gsm_get_ipaddr();
			if(state)
			{
				debug_out("gsm_get_ipaddr success\r\n");
				debug_out(modem.ip);
			}
			else
			{
				debug_out("gsm_get_ipaddr fail\r\n");
			}

			// return semaphore
			xSemaphoreGive( modemUARTSemaphore );
			xSemaphoreGive(lcdSemaphore);

			// delete boot task
			vTaskDelete(bootTaskHandle);	
		}
	}
}

/* monModem */
static void monModem(void * pvParameters)
{
	for(;;)
	{
		// Wait for resource
		if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{
			// ping modem
			if(gsm_ping_modem())
			{
				debug_out("ping ok\r\n");
			}
			// Return semaphore
			xSemaphoreGive( modemUARTSemaphore );		
		}
		vTaskDelay(500);
	}
}

static void display(void * pvParameters)
{
	uint8_t count = 0;
	char buff[20];
	for(;;)
	{
		if( xSemaphoreTake( lcdSemaphore, ( portTickType ) 1 ) == pdTRUE )
		{
			if(count == 0)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print(" GPRS STATUS        ");
				lcd_write_instruction_4d(0xC0);
				lcd_print(modem.ipstate);
				lcd_print("        ");
				count = 1;
			}
			else if(count == 1)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print(" IP ADDRESS        ");
				lcd_write_instruction_4d(0xC0);
				lcd_print(modem.ip);
				lcd_print("        ");
				count = 2;
			}
			else if(count == 2)
			{
				lcd_write_instruction_4d(0x80);
				lcd_print("SIGNAL STRENGTH ");
				lcd_write_instruction_4d(0xC0);
				if(modem.rssi >= 2 && modem.rssi <= 9)
				{
					lcd_print("  Marginal          ");
				}
				else if(modem.rssi >= 10 && modem.rssi <= 14)
				{
					lcd_print("  OK                ");
				}
				else if(modem.rssi >= 15 && modem.rssi <= 19)
				{
					lcd_print("  Good             ");
				}
				else if(modem.rssi >= 20 && modem.rssi <= 30)
				{
					lcd_print("  Excellent         ");
				}
				else
					lcd_print("  Error             ");
				count = 0;
			}	
			xSemaphoreGive(lcdSemaphore);
		}		
		vTaskDelay(5000);
	}
	/*
			------- RSSI ----------
			2 	-109 	Marginal
			3 	-107 	Marginal
			4 	-105 	Marginal
			5 	-103 	Marginal
			6 	-101 	Marginal
			7 	-99 	Marginal
			8 	-97 	Marginal
			9 	-95 	Marginal
			10 	-93 	OK
			11 	-91 	OK
			12 	-89 	OK
			13 	-87 	OK
			14 	-85 	OK
			15 	-83 	Good
			16 	-81 	Good
			17 	-79 	Good
			18 	-77 	Good
			19 	-75 	Good
			20 	-73 	Excellent
			21 	-71 	Excellent
			22 	-69 	Excellent
			23 	-67 	Excellent
			24 	-65 	Excellent
			25 	-63 	Excellent
			26 	-61 	Excellent
			27 	-59 	Excellent
			28 	-57 	Excellent
			29 	-55 	Excellent
			30 	-53 	Excellent
*/
}

static void monZigbee(void * pvParameters)
{
	for(;;)
	{
		if(db0.d_state == 0)
		{
			LPC_GPIO1->FIOSET |= (1 << 19);
		}
		else
		{
			LPC_GPIO1->FIOCLR |= (1 << 19);
		}

		if(db1.d_state == 0)
		{
			LPC_GPIO1->FIOSET |= (1 << 20);
		}
		else
		{
			LPC_GPIO1->FIOCLR |= (1 << 20);
		}

		if(db2.d_state == 0)
		{
			LPC_GPIO1->FIOSET |= (1 << 21);
		}
		else
		{
			LPC_GPIO1->FIOCLR |= (1 << 21);
		}
		
		vTaskDelay(1000);
	}
}

/* establishGPRS */
static void establishGPRS(void * pvParameters)
{
	uint16_t len, resp;
	uint8_t ref = 0;
	for(;;)
	{
		// Wait for resource
		if( xSemaphoreTake( modemUARTSemaphore, ( portTickType ) 10 ) == pdTRUE )
		{

			// query ipstatus
			uint8_t state = gsm_update_ipstatus();
			if(state)
			{
				debug_out("ok\r\n");
				/*if( xSemaphoreTake( lcdSemaphore, ( portTickType ) 1 ) == pdTRUE )
				{

					lcd_write_instruction_4d(0xC0);
					lcd_print(modem.ipstate);
					lcd_print("        ");

					// Return semaphore
					xSemaphoreGive(lcdSemaphore);
				}*/
			}

			// query rssi
			state = gsm_update_rssi();
			if(state)
			{
				debug_out("rssi ok\r\n");
				/*if( xSemaphoreTake( lcdSemaphore, ( portTickType ) 1 ) == pdTRUE )
				{
					sprintf(debug_buff, "%d", modem.rssi);
					lcd_write_instruction_4d(0x80);
					lcd_print(debug_buff);
					lcd_print("        ");

					// Return semaphore
					xSemaphoreGive(lcdSemaphore);
				}*/	
			}

			if(ref == 0)
				state = gsm_http_get("lit-taiga-2854.herokuapp.com","/zigbee/1");
			else if(ref == 1)
				state = gsm_http_get("lit-taiga-2854.herokuapp.com","/zigbee/2");
			else if(ref == 2)
				state = gsm_http_get("lit-taiga-2854.herokuapp.com","/zigbee/3");


			if(state || strstr(modem.ipstate,"CONNECT OK"))
			{
				debug_out("http ok\r\n");

				len = uart3_readline();
				debug_out(uart3_fifo.line);

				uint8_t blank_count = 0;
				debug_out("header\r\n");
				do
				{
					len = uart3_readline();
					//debug_out(uart3_fifo.line);
					if(isblankstr(uart3_fifo.line, len))
					{
						//debug_out("blank\r\n");
						blank_count++;
					}

				}
				while(isblankstr(uart3_fifo.line, len) != 1);

				debug_out("data enter\r\n");
				uint16_t _i = 0;
				while(uart3_fifo.num_bytes > 0)
				{
					char c = uart3_getc();
					modem.httpdata[_i++] = c;
					vTaskDelay(1);
				}
				debug_out("data\r\n");
				modem.httpdata[_i] = '\0';
				//debug_out(modem.httpdata);

				//{"id":1,"created_at":"2015-04-21T16:47:49.393Z","updated_at":"2015-04-21T16:47:49.393Z","ref":1,"d_state":0,"s_val":0}
				
				char * __start = strstr(modem.httpdata, "ref");
				if(__start)
				{
					__start += sizeof("ef\":");

					if(ref == 0)
					{
						db0.ref = __start[0] - 48;
						sprintf(debug_buff,"ref: %d\r\n", db0.ref);
						debug_out(debug_buff);
					}
					else if(ref == 1)
					{
						db1.ref = __start[0] - 48;
						sprintf(debug_buff,"ref: %d\r\n", db1.ref);
						debug_out(debug_buff);
					}
					else if(ref == 2)
					{
						db2.ref = __start[0] - 48;
						sprintf(debug_buff,"ref: %d\r\n", db2.ref);
						debug_out(debug_buff);
					}

				
					__start = strstr(modem.httpdata, "d_state");
					__start += sizeof("_state\":");

					if(ref == 0)
					{
						db0.d_state = __start[0] - 48;
						sprintf(debug_buff,"state: %d\r\n", db0.d_state);
						debug_out(debug_buff);
					}
					else if(ref == 1)
					{
						db1.d_state = __start[0] - 48;
						sprintf(debug_buff,"state: %d\r\n", db1.d_state);
						debug_out(debug_buff);
					}
					else if(ref == 2)
					{
						db2.d_state = __start[0] - 48;
						sprintf(debug_buff,"state: %d\r\n", db2.d_state);
						debug_out(debug_buff);
					}
					

					__start = strstr(modem.httpdata, "s_val");
					__start += sizeof("_val\":");
					char * temp;
					if(ref == 0)
					{
						db0.s_val = strtol(__start, &temp, 10);
						sprintf(debug_buff,"value: %d\r\n", db0.s_val);
						debug_out(debug_buff);
						ref = 1;
					}
					else if(ref == 1)
					{
						db1.s_val = strtol(__start, &temp, 10);
						sprintf(debug_buff,"value: %d\r\n", db1.s_val);
						debug_out(debug_buff);
						ref = 2;
					}
					else if(ref == 2)
					{
						db2.s_val = strtol(__start, &temp, 10);
						sprintf(debug_buff,"value: %d\r\n", db2.s_val);
						debug_out(debug_buff);
						ref = 0;
					}
				}



				uint8_t trials = 0;
				back1:
				state = gsm_tcp_close();
				if(state)
				{
					debug_out("TCP close\r\n");
				}
				else
				{
					debug_out("TCP close fail\r\n");
					trials++;
					if(trials < 10)
					{
						vTaskDelay(100);
						goto back1;
					}
				}
			}

			xSemaphoreGive( modemUARTSemaphore );
		}
		vTaskDelay(500);
	}
}



