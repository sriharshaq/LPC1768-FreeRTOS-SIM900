
#ifndef __config_h__
#define __config_h__

#define __UART0_BAUDRATE 9600
#define __UART1_BAUDRATE 9600
#define __UART3_BAUDRATE 9600

#define debug_out(ptr) 		uart1_print(ptr)
#define debug_puts(ptr, l)	uart1_puts(ptr, l)
#define debug_putc(c)		uart1_putc(c)

#endif