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

/** @brief 
 *  @param 
 *  @return 
 */
void prvSetupHardware(void)
{
	SystemInit();
	SystemCoreClockUpdate();

	/* init uart's */
	uart0_init(__UART0_BAUDRATE);				// Zigbee
	uart1_init(__UART1_BAUDRATE);				// debug port
	uart3_init(__UART3_BAUDRATE);				// modem port

	/* initiate lcd */
	lcd_init();									// init lcd

	/* init buffers for gsm modem */
	gsm_buff_init();

	LPC_GPIO1->FIODIR |= (1 << 19);
	LPC_GPIO1->FIODIR |= (1 << 20);
	LPC_GPIO1->FIODIR |= (1 << 21);

	LPC_GPIO1->FIOSET |= (1 << 19);
	LPC_GPIO1->FIOSET |= (1 << 20);
	LPC_GPIO1->FIOSET |= (1 << 21);
}


uint8_t system_boot(void)
{
	char buff[10];
	prvSetupHardware();

	debug_out("system started\r\n");

	lcd_clearscreen();
	lcd_set_xy(0,0);
	lcd_print("system started", 1);
	lcd_set_xy(0,1);
	lcd_print("booting......", 1);

	_delay_ms(100);

	lcd_set_xy(0,0);
	lcd_print("pinging modem", 1);
	lcd_set_xy(0, 1);
	lcd_print("PING: ", 1);
	lcd_write_character_4d(0 + 48);

	uint8_t j = 0;

	_delay_ms(100);

	/* ping modem for 100 times */
	for(uint8_t i = 1;i < 5;i++)
	{
		if(gsm_ping_modem())
		{
			lcd_set_xy(sizeof("PING: "), 1);
			sprintf(buff, "%d", i);
			lcd_print(buff, 0);
			j++;
		}	
	}

	lcd_set_xy(0,0);
	lcd_print("pinging xbee", 1);
	lcd_set_xy(0, 1);
	lcd_print("PING: ", 1);

	lcd_write_character_4d(0 + 48);

	j = 0;
	_delay_ms(100);

	/* ping modem for 100 times */
	for(uint8_t i = 1;i < 3;i++)
	{
		if(ping_zigbee())
		{
			lcd_set_xy(sizeof("PING: "), 1);
			sprintf(buff, "%d", i);
			lcd_print(buff, 0);
			j++;
			_delay_ms(100);
		}	
	}

	/* check whether ping is failed */
	if(j == 0)
	{
		lcd_set_xy(0, 1);
		lcd_print("xbee ping failed", 1);
		return 0;
	}

	_delay_ms(100);

	/* check network registration status */
	lcd_set_xy(0, 1);
	lcd_print("checking network",1);
	if(gsm_get_network_reg_state())
	{
		lcd_set_xy(0, 1);
		if(modem.nw_reg == 1)
		{
			lcd_print("registered", 1);
		}
		else
		{
			lcd_set_xy(0, 1);
			lcd_print("not registered", 1);		
			return 0;			
		}
	}

	else
	{
		lcd_set_xy(0, 1);
		lcd_print("nw reg fail", 1);		
		return 0;
	}

	_delay_ms(100);

	/* check the operator */
	lcd_set_xy(0, 1);
	lcd_print("checking operator",1);
	if(gsm_get_operator_name())
	{
		lcd_set_xy(0, 1);
		lcd_print(modem.operator_name, 1);
		debug_out(modem.operator_name);
		debug_out("\r\n");
	}

	else
	{
		lcd_set_xy(0, 1);
		lcd_print("opr name fail", 1);		
		return 0;
	}

	_delay_ms(100);

	lcd_set_xy(0, 1);
	lcd_print("checking signal",1);
	if(gsm_get_rssi())
	{
		lcd_set_xy(0, 1);
		lcd_print("qual: ", 1);
	}
	else
	{
		lcd_set_xy(0, 1);
		lcd_print("get signal fail", 1);		
		return 0;
	}

	_delay_ms(100);


	lcd_set_xy(0, 1);
	lcd_print("setting apn",1);
	if(detect_and_set_apn())
	{
		lcd_set_xy(0, 1);
		lcd_print(modem.getapn, 1);
	}

	else
	{
		lcd_set_xy(0, 1);
		lcd_print("apn failed", 1);		
		return 0;
	}

	return 1;
}