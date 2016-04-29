#ifndef __UART_TRI_0_H
#define __UART_TRI_0_H

//#include <stdbool.h>
#include "uart_types.h"


	/************************************************************************/
	/* Definování atmega 48 88 168 328                                      */
	/************************************************************************/
#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__)
	#error "Pozor Pozor nejsou ještì dodelany "
	/************************************************************************/
	/* Definování atmega 8 16 32		                                    */
	/************************************************************************/
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__) || defined(__AVR_ATmega8A__)

	#define UART0_BUFFER_LINEAR_SIZE (0x10)
	#define UART0_BUFFER_PACKET_SIZE (0x10)
	#define UART0_TIMEOUT (5)
	#define UART0_DEFAULT_BAUD 115200
	#define UART0_TX_TIMEOUT (200)
	
	#define UART0_PROC_UDR      UDR
	#define UART0_PROC_UCSRB    UCSRB
	#define UART0_PROC_UCSRA    UCSRA
	#define UART0_PROC_UBRRL    UBRRL
	#define UART0_PROC_RXCIE    RXCIE
	#define UART0_PROC_TXCIE    TXCIE
	#define UART0_PROC_RX_vect  USART_RXC_vect
	#define UART0_PROC_TX_vect  USART_TXC_vect
	#define UART0_PROC_TXEN     TXEN
	#define UART0_PROC_RXEN     RXEN

	#define UART0_TX_ENA  PORTD |= BV(PIND2);
	#define UART0_TX_DIS  PORTD &= ~(BV(PIND2));


	extern volatile byte uart0_status;
	extern volatile Tuartflags uart0_flags;

	#define UART0_BUFFER_LINEAR_SIZE_MAX (UART0_BUFFER_LINEAR_SIZE-1)
	#define UART0_BUFFER_PACKET_SIZE_MAX (UART0_BUFFER_PACKET_SIZE-1)
	
	/************************************************************************/
	/* Definování atmega 64 128			                                    */
	/************************************************************************/
#elif defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__) || defined(__AVR_ATmega128A__)

	
	#define UART0_BUFFER_LINEAR_SIZE (0x10)
	#define UART0_BUFFER_PACKET_SIZE (0x10)
	#define UART0_TIMEOUT (5)
	#define UART0_DEFAULT_BAUD 115200
	#define UART0_TX_TIMEOUT (200)

	#define UART0_PROC_UDR      UDR0
	#define UART0_PROC_UCSRB    UCSR0B
	#define UART0_PROC_UCSRA    UCSR0A
	#define UART0_PROC_UBRRL    UBRR0L
	#define UART0_PROC_RXCIE    RXCIE0
	#define UART0_PROC_TXCIE    TXCIE0
	#define UART0_PROC_RX_vect  USART0_RX_vect
	#define UART0_PROC_TX_vect  USART0_TX_vect
	#define UART0_PROC_TXEN     TXEN0
	#define UART0_PROC_RXEN     RXEN0

	#define UART0_TX_ENA  PORTD |= BV(PIND0);
	#define UART0_TX_DIS  PORTD &= ~(BV(PIND0));


	extern volatile byte uart0_status;
	extern volatile Tuartflags uart0_flags;

	#define UART0_BUFFER_LINEAR_SIZE_MAX (UART0_BUFFER_LINEAR_SIZE-1)
	#define UART0_BUFFER_PACKET_SIZE_MAX (UART0_BUFFER_PACKET_SIZE-1)
	

#else
	# warning "Pozor neni definovan jaky typ mikrokontroleru je pouzit"
#endif




/*
 * Perform UART startup initialization.
 */
void uart0_init(void);
void uart0_process(void);
void uart0_set_baud(byte baud);
void uart0_ISR_timer(void);
byte * uart0_get_data_begin(void);
byte uart0_get_data_end(void);
void uart0_put_data(byte * dataptr);
byte uart_pac_rx_empty(void);
byte uart_pac_rx_size(void);


#endif /*_UART_TRI_0_H_*/