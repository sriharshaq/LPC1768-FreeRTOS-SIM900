
#ifndef __uart_h__
#define __uart_h__

#include <stdint.h>

/******* Enable/Disable uarts     */
#define __UART0_ENABLED
//#define __UART1_ENABLED
//#define __UART2_ENABLED
#define __UART3_ENABLED
/**********************************/

/******* Enable/Disable interrupts */
#define UART0_INTERRUPT_ENABLED
//#define UART1_INTERRUPT_ENABLED
//#define UART2_INTERRUPT_ENABLED
#define UART3_INTERRUPT_ENABLED
/**********************************/

/*************************************/
#define IER_RBR		0x01
#define IER_THRE	0x02
#define IER_RLS		0x04
#define IIR_PEND	0x01
#define IIR_RLS		0x03
#define IIR_RDA		0x02
#define IIR_CTI		0x06
#define IIR_THRE	0x01
#define LSR_RDR		0x01
#define LSR_OE		0x02
#define LSR_PE		0x04
#define LSR_FE		0x08
#define LSR_BI		0x10
#define LSR_THRE	0x20
#define LSR_TEMT	0x40
#define LSR_RXFE	0x80
#define BUFSIZE		0x40
#define TEMT 		(1 << 6) 
/*************************************/


#define LF '\n'

/* UART RING BUFFER STRUCTURE */
typedef struct
{
	volatile uint32_t 			i_first;
	volatile uint32_t 			i_last;
	volatile uint8_t			rx_ovf;
	volatile uint8_t			fifo_full;
	volatile uint32_t			num_bytes;
	volatile uint8_t			rx_not_empty;
	volatile uint8_t			rx_status;
	volatile uint32_t			error_count;
	volatile uint32_t			fifo_size;
	volatile uint32_t			line_size;
	char					*	rx_fifo;
	char					*	line;
}UART_RING_BUFFER;


#ifdef __UART0_ENABLED
	#ifdef 	UART0_INTERRUPT_ENABLED
		#define UART0_RX_FIFO_SIZE			1024
		#define UART0_LINE_SIZE				512

		extern 	UART_RING_BUFFER 			uart0_fifo;
		extern void 						uart0_init(uint32_t);
		extern char 						uart0_getc(void);
		extern void							uart0_putc(char);	
		extern int32_t						uart0_readline(void);
	#endif
#endif

#ifdef __UART1_ENABLED
	#ifdef 	UART1_INTERRUPT_ENABLED
		#define UART1_RX_FIFO_SIZE			1024
		#define UART1_LINE_SIZE				512

		extern 	UART_RING_BUFFER 			uart1_fifo;
		extern void 						uart1_init(uint32_t);
		extern char 						uart1_getc(void);
		extern void							uart1_putc(char);	
		extern int32_t						uart1_readline(void);
	#endif
#endif

#ifdef __UART2_ENABLED
	#ifdef 	UART2_INTERRUPT_ENABLED
		#define UART2_RX_FIFO_SIZE			1024
		#define UART2_LINE_SIZE				512

		extern 	UART_RING_BUFFER 			uart2_fifo;
		extern void 						uart2_init(uint32_t);
		extern char 						uart2_getc(void);
		extern void							uart2_putc(char);	
		extern int32_t						uart2_readline(void);
	#endif
#endif

#ifdef __UART3_ENABLED
	#ifdef 	UART3_INTERRUPT_ENABLED
		#define UART3_RX_FIFO_SIZE			1024
		#define UART3_LINE_SIZE				512

		extern 	UART_RING_BUFFER 			uart3_fifo;
		extern void 						uart3_init(uint32_t);
		extern char 						uart3_getc(void);
		extern void							uart3_putc(char);
		extern int32_t						uart3_readline(void);	
	#endif
#endif

#endif