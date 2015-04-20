
#include "stdint.h"
#include "stdlib.h"
#include "system_LPC17xx.h"
#include "LPC17xx.h"
#include "uart.h"


#ifdef __UART0_ENABLED
	void uart0_init(uint32_t __baudrate)
	{
		uint32_t Fdiv;
		uint32_t pclkdiv, pclk;	

		pclkdiv = (LPC_SC->PCLKSEL0 >> 6) & 0x03;

		switch ( pclkdiv )
		{
			case 0x00:
			default:
				pclk = SystemCoreClock / 4;
				break;
			case 0x01:
				pclk = SystemCoreClock;
				break;
			case 0x02:
				pclk = SystemCoreClock / 2;
				break;
			case 0x03:
				pclk = SystemCoreClock / 8;
				break;
		}

		LPC_PINCON->PINSEL0 &= ~0x000000F0;
		LPC_PINCON->PINSEL0 |= 0x00000050; /* RxD0 is P0.3 and TxD0 is P0.2 */

		LPC_UART0->LCR 			= 0x83; /* 8 bits, no Parity, 1 Stop bit */
		Fdiv 					= ( pclk / 16 ) / __baudrate ; /*baud rate */

		LPC_UART0->DLM 			= Fdiv / 256;
		LPC_UART0->DLL 			= Fdiv % 256;

		LPC_UART0->LCR 			= 0x03; /* DLAB = 0 */
		LPC_UART0->FCR 			= 0x07; /* Enable and reset TX and RX FIFO. */

		#ifdef UART0_INTERRUPT_ENABLED
			NVIC_EnableIRQ(UART0_IRQn);
			LPC_UART0->IER = IER_RBR | IER_RLS; /* Enable UART0 interrupt */

			/* Init buffers */
			uart0_fifo.i_first 			= 0;			
	 		uart0_fifo.i_last 			= 0;				
	 		uart0_fifo.rx_ovf 			= 0;
	 		uart0_fifo.fifo_full 		= 0;
	 		uart0_fifo.num_bytes 		= 0;
	 		uart0_fifo.rx_not_empty 	= 0;
	 		uart0_fifo.error_count		= 0;
	 		uart0_fifo.fifo_size 		= UART0_RX_FIFO_SIZE;
	 		uart0_fifo.rx_fifo 			= (char *) malloc(uart0_fifo.fifo_size);	
	 		uart0_fifo.line_size 		= UART0_LINE_SIZE;
	 		uart0_fifo.line				= (char * ) malloc(uart0_fifo.line_size);
		#endif
	}

	void uart0_putc(char c)
	{
		LPC_UART0->THR = c;
		while(!((LPC_UART0->LSR) & TEMT));
	}

	#ifdef UART0_INTERRUPT_ENABLED

		/* Declare Ring Buffer */
		UART_RING_BUFFER uart0_fifo;

		char uart0_getc(void)
		{
			char _byte = '\0';
			if(uart0_fifo.num_bytes == uart0_fifo.fifo_size)
			{
				uart0_fifo.fifo_full = 0;
			}

			if(uart0_fifo.num_bytes > 0)
			{
				_byte = uart0_fifo.rx_fifo[uart0_fifo.i_first];
				uart0_fifo.i_first++;
				uart0_fifo.num_bytes--;
			}
			else
			{
				uart0_fifo.rx_not_empty = 0;
			}

			if(uart0_fifo.i_first == uart0_fifo.fifo_size)
			{
				uart0_fifo.i_first = 0;
			}
			return _byte;
		}

		int32_t uart0_readline(void)
		{
			char c = '\0';
			char i = 0;
			while(c != LF)
			{
				if(uart0_fifo.num_bytes > 0){
					c = uart0_getc();
					uart0_fifo.line[i] = c;
					i++;
				}
			}
			c = '\0';
			uart0_fifo.line[i] = c;
			return i;
		}

		void UART0_IRQHandler(void)
		{
			uint8_t IIRValue, LSRValue;
			IIRValue 		= LPC_UART0->IIR;
			IIRValue 		>>= 1; /* skip pending bit in IIR */
			IIRValue	 	&= 0x07; /* check bit 1~3, interrupt identification */
			char c 			= '\0';

			// If RLS
			if ( IIRValue == IIR_RLS )
			{
				LSRValue = LPC_UART0->LSR;

				/* There are errors or break interrupt */
				/* Read LSR will clear the interrupt */
				if ( LSRValue & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI) )
				{
					uart0_fifo.rx_status = LSRValue;
					// Read to clear interrupt
					char dummy = LPC_UART0->RBR;
					dummy = '\0';

					/* Incriment Error Count */
					uart0_fifo.error_count++;
					return;
				}

				/* If no error on RLS, normal ready, save into the data buffer. */
				/* Note: read RBR will clear the interrupt */
				if ( LSRValue & LSR_RDR )
				{
					c = LPC_UART0->RBR;
				}
			}

			/* Receive Data Available */
			else if ( IIRValue == IIR_RDA )
			{
				c = LPC_UART0->RBR;
			}

			// Timeout
		 	else if ( IIRValue == IIR_CTI ) 	/* Character timeout indicator */
			{
				/* Character Time-out indicator */
				uart0_fifo.rx_status |= 0x100; 		/* Bit 9 as the CTI error */
				uart0_fifo.error_count++;
			}

			if(c != '\0')
			{
				if(uart0_fifo.num_bytes == uart0_fifo.fifo_size)
				{
					/* Buffer Overflow */
					uart0_fifo.rx_ovf = 1;
				}
				else if(uart0_fifo.num_bytes < uart0_fifo.fifo_size)
				{
					uart0_fifo.rx_fifo[uart0_fifo.i_last] = c;
					uart0_fifo.i_last++;
					uart0_fifo.num_bytes++;
				}
				if(uart0_fifo.num_bytes == uart0_fifo.fifo_size)
				{
					uart0_fifo.fifo_full = 1;
				}
				if(uart0_fifo.i_last == uart0_fifo.fifo_size)
				{
					uart0_fifo.i_last = 0;
				}
				uart0_fifo.rx_not_empty = 1;
			}
		}
	#endif
#endif

#ifdef __UART1_ENABLED
	void uart1_init(uint32_t __baudrate)
	{
		uint32_t Fdiv;
		uint32_t pclkdiv, pclk;	

		pclkdiv = (LPC_SC->PCLKSEL0 >> 8) & 0x03;

		switch ( pclkdiv )
		{
			case 0x00:
			default:
				pclk = SystemCoreClock / 4;
				break;
			case 0x01:
				pclk = SystemCoreClock;
				break;
			case 0x02:
				pclk = SystemCoreClock / 2;
				break;
			case 0x03:
				pclk = SystemCoreClock / 8;
				break;
		}

		LPC_PINCON->PINSEL4 &= ~0x0000000F;
		LPC_PINCON->PINSEL4 |= 0x0000000A; /* Enable RxD1 P2.1, TxD1 P2.0 */

		LPC_UART1->LCR 			= 0x83; /* 8 bits, no Parity, 1 Stop bit */
		Fdiv 					= ( pclk / 16 ) / __baudrate ; /*baud rate */

		LPC_UART1->DLM 			= Fdiv / 256;
		LPC_UART1->DLL 			= Fdiv % 256;

		LPC_UART1->LCR 			= 0x03; /* DLAB = 0 */
		LPC_UART1->FCR 			= 0x07; /* Enable and reset TX and RX FIFO. */

		#ifdef UART1_INTERRUPT_ENABLED
			NVIC_EnableIRQ(UART1_IRQn);
			LPC_UART1->IER = IER_RBR | IER_RLS; /* Enable UART0 interrupt */

			/* Init buffers */
			uart1_fifo.i_first 			= 0;			
	 		uart1_fifo.i_last 			= 0;				
	 		uart1_fifo.rx_ovf 			= 0;
	 		uart1_fifo.fifo_full 		= 0;
	 		uart1_fifo.num_bytes 		= 0;
	 		uart1_fifo.rx_not_empty 	= 0;
	 		uart1_fifo.error_count		= 0;
	 		uart1_fifo.fifo_size		= UART1_RX_FIFO_SIZE;
	 		uart1_fifo.rx_fifo 			= (char *) malloc(uart1_fifo.fifo_size);	
	 		uart1_fifo.line_size 		= UART1_LINE_SIZE;
	 		uart1_fifo.line				= (char * ) malloc(uart1_fifo.line_size);
		#endif
	}

	void uart1_putc(char c)
	{
		LPC_UART1->THR = c;
		while(!((LPC_UART1->LSR) & TEMT));
	}

	#ifdef UART1_INTERRUPT_ENABLED
		/* Declare Ring Buffer */
		UART_RING_BUFFER uart1_fifo;

		char uart1_getc(void)
		{
			char _byte = '\0';
			if(uart1_fifo.num_bytes == uart1_fifo.fifo_size)
			{
				uart1_fifo.fifo_full = 0;
			}

			if(uart1_fifo.num_bytes > 0)
			{
				_byte = uart1_fifo.rx_fifo[uart1_fifo.i_first];
				uart1_fifo.i_first++;
				uart1_fifo.num_bytes--;
			}
			else
			{
				uart1_fifo.rx_not_empty = 0;
			}

			if(uart1_fifo.i_first == uart1_fifo.fifo_size)
			{
				uart1_fifo.i_first = 0;
			}
			return _byte;		
		}

		int32_t uart1_readline(void)
		{
			char c = '\0';
			char i = 0;
			while(c != LF)
			{
				if(uart1_fifo.num_bytes > 0){
					c = uart1_getc();
					memcpy(&uart1_fifo.line[i], c, 1);
					i++;
				}
			}
			c = '\0';
			memcpy(&uart1_fifo.line[i], c, 1);
			return i;
		}

		void UART1_IRQHandler(void)
		{
			uint8_t IIRValue, LSRValue;
			IIRValue 		= LPC_UART3->IIR;
			IIRValue 		>>= 1; /* skip pending bit in IIR */
			IIRValue	 	&= 0x07; /* check bit 1~3, interrupt identification */
			char c 			= '\0';

			// If RLS
			if ( IIRValue == IIR_RLS )
			{
				LSRValue = LPC_UART1->LSR;

				/* There are errors or break interrupt */
				/* Read LSR will clear the interrupt */
				if ( LSRValue & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI) )
				{
					uart1_fifo.rx_status = LSRValue;
					// Read to clear interrupt
					char dummy = LPC_UART1->RBR;
					dummy = '\0';
					uart1_fifo.error_count++;
					return;
				}

				/* If no error on RLS, normal ready, save into the data buffer. */
				/* Note: read RBR will clear the interrupt */
				if ( LSRValue & LSR_RDR )
				{
					c = LPC_UART1->RBR;
				}
			}

			/* Receive Data Available */
			else if ( IIRValue == IIR_RDA )
			{
				c = LPC_UART1->RBR;
			}

			// Timeout
		 	else if ( IIRValue == IIR_CTI ) 	/* Character timeout indicator */
			{
				/* Character Time-out indicator */
				uart1_fifo.rx_status |= 0x100; 		/* Bit 9 as the CTI error */
				uart1_fifo.error_count++;
			}

			if(c != '\0')
			{
				if(uart1_fifo.num_bytes == uart1_fifo.fifo_size)
				{
					uart1_fifo.rx_ovf = 1;
				}
				else if(uart1_fifo.num_bytes < uart1_fifo.fifo_size)
				{
					uart1_fifo.rx_fifo[uart1_fifo.i_last] = c;
					uart1_fifo.i_last++;
					uart1_fifo.num_bytes++;
				}
				if(uart1_fifo.num_bytes == uart1_fifo.fifo_size)
				{
					uart1_fifo.fifo_full = 1;
				}
				if(uart1_fifo.i_last == uart1_fifo.fifo_size)
				{
					uart1_fifo.i_last = 0;
				}
				uart1_fifo.rx_not_empty = 1;
			}
		}
	#endif
#endif


#ifdef __UART2_ENABLED
	void uart2_init(uint32_t __baudrate)
	{
		uint32_t Fdiv;
		uint32_t pclkdiv, pclk;	

		pclkdiv = (LPC_SC->PCLKSEL0 >> 8) & 0x03;

		switch ( pclkdiv )
		{
			case 0x00:
			default:
				pclk = SystemCoreClock / 4;
				break;
			case 0x01:
				pclk = SystemCoreClock;
				break;
			case 0x02:
				pclk = SystemCoreClock / 2;
				break;
			case 0x03:
				pclk = SystemCoreClock / 8;
				break;
		}

		LPC_PINCON->PINSEL4 &= ~0x0000000F;
		LPC_PINCON->PINSEL4 |= 0x0000000A; /* Enable RxD1 P2.1, TxD1 P2.0 */

		LPC_UART2->LCR 			= 0x83; /* 8 bits, no Parity, 1 Stop bit */
		Fdiv 					= ( pclk / 16 ) / __baudrate ; /*baud rate */

		LPC_UART2->DLM 			= Fdiv / 256;
		LPC_UART2->DLL 			= Fdiv % 256;

		LPC_UART2->LCR 			= 0x03; /* DLAB = 0 */
		LPC_UART2->FCR 			= 0x07; /* Enable and reset TX and RX FIFO. */

		#ifdef UART2_INTERRUPT_ENABLED
			NVIC_EnableIRQ(UART2_IRQn);
			LPC_UART2->IER = IER_RBR | IER_RLS; /* Enable UART0 interrupt */

			/* Init buffers */
			uart2_fifo.i_first 			= 0;			
	 		uart2_fifo.i_last 			= 0;				
	 		uart2_fifo.rx_ovf 			= 0;
	 		uart2_fifo.fifo_full 		= 0;
	 		uart2_fifo.num_bytes 		= 0;
	 		uart2_fifo.rx_not_empty 	= 0;
	 		uart2_fifo.error_count		= 0;
	 		uart2_fifo.fifo_size		= UART2_RX_FIFO_SIZE;
	 		uart2_fifo.rx_fifo 			= (char *) malloc(uart2_fifo.fifo_size);	
	 		uart2_fifo.line_size 		= UART2_LINE_SIZE;
	 		uart2_fifo.line				= (char * ) malloc(uart2_fifo.line_size);
		#endif
	}

	void uart2_putc(char c)
	{
		LPC_UART2->THR = c;
		while(!((LPC_UART2->LSR) & TEMT));
	}

	#ifdef UART2_INTERRUPT_ENABLED

		/* Declare Ring Buffer */
		UART_RING_BUFFER uart2_fifo;

		char uart2_getc(void)
		{
			char _byte = '\0';
			if(uart2_fifo.num_bytes == uart2_fifo.fifo_size)
			{
				uart2_fifo.fifo_full = 0;
			}

			if(uart2_fifo.num_bytes > 0)
			{
				_byte = uart2_fifo.rx_fifo[uart2_fifo.i_first];
				uart2_fifo.i_first++;
				uart2_fifo.num_bytes--;
			}
			else
			{
				uart2_fifo.rx_not_empty = 0;
			}

			if(uart2_fifo.i_first == uart2_fifo.fifo_size)
			{
				uart2_fifo.i_first = 0;
			}
			return _byte;	
		}

		int32_t uart2_readline(void)
		{
			char c = '\0';
			char i = 0;
			while(c != LF)
			{
				if(uart2_fifo.num_bytes > 0){
					c = uart2_getc();
					memcpy(&uart2_fifo.line[i], c, 1);
					i++;
				}
			}
			c = '\0';
			memcpy(&uart2_fifo.line[i], c, 1);
			return i;
		}

		void UART2_IRQHandler(void)
		{
			uint8_t IIRValue, LSRValue;
			IIRValue 		= LPC_UART2->IIR;
			IIRValue 		>>= 1; /* skip pending bit in IIR */
			IIRValue	 	&= 0x07; /* check bit 1~3, interrupt identification */
			char c 			= '\0';

			// If RLS
			if ( IIRValue == IIR_RLS )
			{
				LSRValue = LPC_UART2->LSR;

				/* There are errors or break interrupt */
				/* Read LSR will clear the interrupt */
				if ( LSRValue & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI) )
				{
					uart2_fifo.rx_status = LSRValue;
					// Read to clear interrupt
					char dummy = LPC_UART2->RBR;
					dummy = '\0';
					uart2_fifo.error_count++;
					return;
				}

				/* If no error on RLS, normal ready, save into the data buffer. */
				/* Note: read RBR will clear the interrupt */
				if ( LSRValue & LSR_RDR )
				{
					c = LPC_UART2->RBR;
				}
			}

			/* Receive Data Available */
			else if ( IIRValue == IIR_RDA )
			{
				c = LPC_UART2->RBR;
			}
			// Timeout
		 	else if ( IIRValue == IIR_CTI ) 	/* Character timeout indicator */
			{
				/* Character Time-out indicator */
				uart2_fifo.rx_status |= 0x100; 		/* Bit 9 as the CTI error */
				uart2_fifo.error_count++;
			}

			if(c != '\0')
			{
				if(uart2_fifo.num_bytes == uart2_fifo.fifo_size)
				{
					uart2_fifo.rx_ovf = 1;
				}
				else if(uart2_fifo.num_bytes < uart2_fifo.fifo_size)
				{
					uart2_fifo.rx_fifo[uart2_fifo.i_last] = c;
					uart2_fifo.i_last++;
					uart2_fifo.num_bytes++;
				}
				if(uart2_fifo.num_bytes == uart2_fifo.fifo_size)
				{
					uart2_fifo.fifo_full = 1;
				}
				if(uart2_fifo.i_last == uart2_fifo.fifo_size)
				{
					uart2_fifo.i_last = 0;
				}
				uart2_fifo.rx_not_empty = 1;
			}
		}
	#endif
#endif


#ifdef __UART3_ENABLED
	void uart3_init(uint32_t __baudrate)
	{
		uint32_t Fdiv;
		uint32_t pclkdiv, pclk;	

		pclkdiv = (LPC_SC->PCLKSEL1 >> 18) & 0x03;

		switch ( pclkdiv )
		{
			case 0x00:
			default:
				pclk = SystemCoreClock / 4;
				break;
			case 0x01:
				pclk = SystemCoreClock;
				break;
			case 0x02:
				pclk = SystemCoreClock / 2;
				break;
			case 0x03:
				pclk = SystemCoreClock / 8;
				break;
		}

		// Turn on UART3 peripheral clock
		LPC_SC->PCLKSEL1 &= ~(3 << 18);
		LPC_SC->PCLKSEL1 |=  (0 << 18);		// PCLK_periph = CCLK/4

		LPC_SC->PCONP |= (1 << 25);

		// Set PINSEL0 so that P0.0 = TXD3, P0.1 = RXD3
		LPC_PINCON->PINSEL0 &= ~0xf;
		LPC_PINCON->PINSEL0 |= ((1 << 1) | (1 << 3));

		LPC_UART3->LCR 				= 0x83;		// 8 bits, no Parity, 1 Stop bit, DLAB=1
	    Fdiv 						= ( pclk / 16 ) / __baudrate ;	// Set baud rate
	    LPC_UART3->DLM 				= Fdiv / 256;
	    LPC_UART3->DLL 				= Fdiv % 256;
	    LPC_UART3->ACR 				= 0x00;		// Disable autobaud
	    LPC_UART3->LCR 				= 0x03;		// 8 bits, no Parity, 1 Stop bit DLAB = 0
	    LPC_UART3->FCR 				= 0x07;		// Enable and reset TX and RX FIFO

		#ifdef UART3_INTERRUPT_ENABLED
			NVIC_EnableIRQ(UART3_IRQn);
			LPC_UART3->IER = IER_RBR | IER_RLS; /* Enable UART0 interrupt */

			/* Init buffers */
			uart3_fifo.i_first 			= 0;			
	 		uart3_fifo.i_last 			= 0;				
	 		uart3_fifo.rx_ovf 			= 0;
	 		uart3_fifo.fifo_full 		= 0;
	 		uart3_fifo.num_bytes 		= 0;
	 		uart3_fifo.rx_not_empty 	= 0;
	 		uart3_fifo.error_count		= 0;
	 		uart3_fifo.fifo_size		= UART3_RX_FIFO_SIZE;
	 		uart3_fifo.rx_fifo 			= (char *) malloc(uart3_fifo.fifo_size);	
	 		uart3_fifo.line_size 		= UART3_LINE_SIZE;
	 		uart3_fifo.line				= (char * ) malloc(uart3_fifo.line_size);
		#endif
	}

	void uart3_putc(char c)
	{
		LPC_UART3->THR = c;
		while(!((LPC_UART3->LSR) & TEMT));
	}

	#ifdef UART3_INTERRUPT_ENABLED

		/* Declare Ring Buffer */
		UART_RING_BUFFER uart3_fifo;

		char uart3_getc(void)
		{
			char _byte = '\0';
			if(uart3_fifo.num_bytes == uart3_fifo.fifo_size)
			{
				uart3_fifo.fifo_full = 0;
			}

			if(uart3_fifo.num_bytes > 0)
			{
				_byte = uart3_fifo.rx_fifo[uart3_fifo.i_first];
				uart3_fifo.i_first++;
				uart3_fifo.num_bytes--;
			}
			else
			{
				uart3_fifo.rx_not_empty = 0;
			}

			if(uart3_fifo.i_first == uart3_fifo.fifo_size)
			{
				uart3_fifo.i_first = 0;
			}
			return _byte;	
		}

		int32_t uart3_readline(void)
		{
			char c = '\0';
			char i = 0;
			while(c != LF)
			{
				if(uart3_fifo.num_bytes > 0){
					c = uart3_getc();
					uart3_fifo.line[i] = c;
					i++;
				}
			}
			c = '\0';
			uart3_fifo.line[i] = c;
			return i;
		}

		void UART3_IRQHandler(void)
		{
			uint8_t IIRValue, LSRValue;
			IIRValue 		= LPC_UART3->IIR;
			IIRValue 		>>= 1; /* skip pending bit in IIR */
			IIRValue	 	&= 0x07; /* check bit 1~3, interrupt identification */
			char c 			= '\0';

			// If RLS
			if ( IIRValue == IIR_RLS )
			{
				LSRValue = LPC_UART3->LSR;

				/* There are errors or break interrupt */
				/* Read LSR will clear the interrupt */
				if ( LSRValue & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI) )
				{
					uart3_fifo.rx_status = LSRValue;
					// Read to clear interrupt
					char dummy = LPC_UART3->RBR;
					dummy = '\0';
					uart3_fifo.error_count++;
					return;
				}

				/* If no error on RLS, normal ready, save into the data buffer. */
				/* Note: read RBR will clear the interrupt */
				if ( LSRValue & LSR_RDR )
				{
					c = LPC_UART3->RBR;
				}
			}

			/* Receive Data Available */
			else if ( IIRValue == IIR_RDA )
			{
				c = LPC_UART3->RBR;
			}
			// Timeout
		 	else if ( IIRValue == IIR_CTI ) 	/* Character timeout indicator */
			{
				/* Character Time-out indicator */
				uart3_fifo.rx_status |= 0x100; 		/* Bit 9 as the CTI error */
				uart3_fifo.error_count++;
			}

			if(c != '\0')
			{
				if(uart3_fifo.num_bytes == uart3_fifo.fifo_size)
				{
					uart3_fifo.rx_ovf = 1;
				}
				else if(uart3_fifo.num_bytes < uart3_fifo.fifo_size)
				{
					uart3_fifo.rx_fifo[uart3_fifo.i_last] = c;
					uart3_fifo.i_last++;
					uart3_fifo.num_bytes++;
				}
				if(uart3_fifo.num_bytes == uart3_fifo.fifo_size)
				{
					uart3_fifo.fifo_full = 1;
				}
				if(uart3_fifo.i_last == uart3_fifo.fifo_size)
				{
					uart3_fifo.i_last = 0;
				}
				uart3_fifo.rx_not_empty = 1;
			}
		}
	#endif
#endif

