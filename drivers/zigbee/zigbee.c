
#include "stdint.h"
#include "stdlib.h"
#include "uart.h"
#include "misc.h"
#include "lcd.h"
#include "delay.h"
#include "zigbee.h"
#include "system_LPC17xx.h"
#include "LPC17xx.h"
#include "stdio.h"
#include "syscalls.h"
#include "string.h"
#include "config.h"

int8_t send_to_end_device(char * addrH,char * addrL, char * dat)
{
	uart0_flushrx();
	/* enter to command mode */
	zigbee_out("+++");

	uint8_t len = zigbee_readline();
	if(len != 0 && strstr(uart0_fifo.line, "OK"))
	{
		zigbee_out("ATDL ");
		zigbee_out(addrL);
		zigbee_out("\r");
		len = zigbee_readline();
		if(len != 0 && strstr(uart0_fifo.line, "OK"))
		{
			zigbee_out("ATDH ");
			zigbee_out(addrH);
			zigbee_out("\r");
			len = zigbee_readline();
			if(len != 0 && strstr(uart0_fifo.line, "OK"))
			{
				zigbee_out("ATCN\r");
				len = zigbee_readline();
				if(len != 0 && strstr(uart0_fifo.line, "OK"))
				{
					zigbee_out(dat);
					return 1;
				}
			}	
		}
	}
	return 0;
}

int8_t ping_zigbee(void)
{
	uart0_flushrx();
	/* enter to command mode */
	zigbee_out("+++");	
	uint8_t len = zigbee_readline();
	debug_out(uart0_fifo.line);
	debug_out("\r\n");
	if(len != 0 && strstr(uart0_fifo.line, "OK"))
	{
		zigbee_out("ATCN\r");
		len = zigbee_readline();
		debug_out(uart0_fifo.line);
		debug_out("\r\n");
		if(len != 0 && strstr(uart0_fifo.line, "OK")){
			return 1;
		}	
	}
	return 0;
}