#ifndef __COMMON_DEF
#define __COMMON_DEF

/* CPU frequency */
#define F_CPU 14745600UL

// cpu cycles per microsecond
#define CYCLES_PER_US ((F_CPU+500000)/1000000)


#define CMD_NULL	0	// nic
#define CMD_IDENT	1	// get identification string
#define CMD_SEND	2	// send serial data
#define CMD_RCV		3	// receive serial data
#define CMD_FLUSH	4 	// flush Rx buffer
#define CMD_STATUS	5	// get status
#define CMD_STATUS_C	6
#define CMD_RCV_SIZE	7	// get Rx buffer length
#define CMD_BAUD_RATE	8	// set baud rate
#define CMD_RTS		9	// set RTS signal state
#define CMD_CTS		10	// get CTS signal state

#define RTS_PORT	PORTD
#define RTS_PIN		PD2
#define CTS_PORT	PIND
#define CTS_PIN		PD3

#define UART_BAUD 9600

/*
status byte:
b0 - pending received data
b1 - sending in progress
b2 - rx data overflow
b3 - tx data overflow
b4 -
b5 -
b6 -
b7 - module not exist (always 0)
*/
#endif
