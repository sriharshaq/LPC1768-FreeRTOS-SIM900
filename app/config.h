
#ifndef __config_h__
#define __config_h__

#define URL 	"http://lit-taiga-2854.herokuapp.com"
#define BASE	"/gateway/"

#define 	__DEBUG_MESSAGES__

#define __UART0_BAUDRATE 9600
#define __UART1_BAUDRATE 9600
#define __UART3_BAUDRATE 9600

#define debug_out(ptr) 		uart1_print(ptr)
#define debug_puts(ptr, l)	uart1_puts(ptr, l)
#define debug_putc(c)		uart1_putc(c)

typedef struct
{
	char card[10];
}CardType_t;

typedef struct 
{
	char card[10];
}KeypadType_t;

#endif