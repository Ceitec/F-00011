#ifndef UART_TRI_TYPES_H
#define UART_TRI_TYPES_H

typedef struct {
  byte txing : 1; // data transmitting
  byte rxing : 1; // data receiving
  byte wait_tx : 1; // waiting for response
  byte data_received : 1; // valid data received
  byte data_receive_error : 1; // some received data was not valid

} Tuartflags;

#endif