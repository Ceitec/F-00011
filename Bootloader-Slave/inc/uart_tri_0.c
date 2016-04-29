#include "common_defs.h"
#include "defines.h"
//#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Tribus_types.h"
#include "uart_types.h"
#include "uart_tri_0.h"

/*


       /-<-uart0_buf_tx[9]---<-uart0_buf_pac_tx
       |
BUS --<
       |
       \->-uart0_buf_rx[moc]->-uart0_buf_pac_rx


*/


// linear char buffers
volatile byte uart0_buf_rx[UART0_BUFFER_LINEAR_SIZE];
volatile byte uart0_buf_rx_ptr_b = 0;
volatile byte uart0_buf_rx_ptr_e = 0;
volatile byte uart0_buf_tx[9];  // buffer na zprávu, co se má odeslat
volatile byte uart0_buf_tx_ptr = 0; // fixed 0 - 8

// packet buffers
Ttripac uart0_buf_pac_tx[UART0_BUFFER_PACKET_SIZE];
volatile byte uart0_buf_pac_tx_ptr_b = 0;
volatile byte uart0_buf_pac_tx_ptr_e = 0;
Ttripac uart0_buf_pac_rx[UART0_BUFFER_PACKET_SIZE];
volatile byte uart0_buf_pac_rx_ptr_b = 0;
volatile byte uart0_buf_pac_rx_ptr_e = 0;

//volatile byte uart0_vysilani = false;
//volatile byte uart0_prijato = false;
volatile byte uart0_rx_timeout = 0; // kontrola návaznosti pøíchozích dat
volatile byte uart0_tx_timeout = 0; // mezera mezi rámci
volatile byte uart0_status = 0;
volatile Tuartflags uart0_flags = {0};
// buf_rx - linear (cyclic) receive buffer
// empty    -> b = e
// length   ->   = (e - b) & UART_BUFFER_SIZE
// free spc ->   = (b - e) & UART_BUFFER_SIZE
// full     -> b = (e + 1) & UART_BUFFER_SIZE
// write    -> e++, write *e
// read     -> b++, read *b

/******************************************************/
// Useful functions
/******************************************************/
inline byte uart_rx_size(void)
{
  return ((uart0_buf_rx_ptr_e - uart0_buf_rx_ptr_b) & UART0_BUFFER_LINEAR_SIZE_MAX);
}

inline byte uart_tx_size(void)
{
  return 9;
}

inline byte uart_rx_empty(void)
{
  return (uart0_buf_rx_ptr_e == uart0_buf_rx_ptr_b);
}

inline void uart_tx_flush(void)
{
  uart0_buf_tx_ptr = 0;
}

inline void uart_rx_flush(void)
{
  uart0_buf_rx_ptr_b = uart0_buf_rx_ptr_e;
}

inline byte uart_pac_rx_empty(void)
{
  return (uart0_buf_pac_rx_ptr_e == uart0_buf_pac_rx_ptr_b);
}

inline byte uart_pac_rx_size(void)
{
  return ((uart0_buf_pac_rx_ptr_e - uart0_buf_pac_rx_ptr_b) & UART0_BUFFER_PACKET_SIZE_MAX);
}

inline byte uart_pac_tx_empty(void)
{
  return (uart0_buf_pac_tx_ptr_e == uart0_buf_pac_tx_ptr_b);
}


/******************************************************/
// buffer logic
/******************************************************/

//----------------------------------------------------------
// get single char from RX linear buffer
char uart_get_char(void)
{
  // read     -> b++, read *b
  // interrupt safe -> read *(b+1); b++
  byte ptr;
  byte res;
  
  if (uart_rx_empty()) return 0;

  ptr = (uart0_buf_rx_ptr_b+1) & UART0_BUFFER_LINEAR_SIZE_MAX;
  res = uart0_buf_rx[ptr];
  uart0_buf_rx_ptr_b = ptr;
  return res;
  
}

//----------------------------------------------------------
// put single char in RX linear buffer
void uart_receive_char(char dat)
{
  // write    -> e++, write *e
  // interrupt safe -> write *(e+1); e++
  byte ptr;

  ptr = (uart0_buf_rx_ptr_e+1) & UART0_BUFFER_LINEAR_SIZE_MAX;
  uart0_buf_rx[ptr] = dat;
  uart0_buf_rx_ptr_e = ptr;
}

//----------------------------------------------------------
// get single char from TX buffer (frame)
char uart_send_char(void)
{
  byte ptr;
  byte res;

  ptr = uart0_buf_tx_ptr;
  res = uart0_buf_tx[ptr];
  uart0_buf_tx_ptr = ptr+1;
  return res;
}

//----------------------------------------------------------



//----------------------------------------------------------

//----------------------------------------------------------


//----------------------------------------------------------
/******************************************************/
// start sending TX buffer
/******************************************************/
void uart_send(void)
{
  if (uart0_flags.txing == FALSE) {
    // is some data in buffer ?
    uart0_flags.txing = TRUE;
    //uart0_tx_timeout = UART0_TX_TIMEOUT;
    UART0_TX_ENA;  // tx mode
    uart0_buf_tx_ptr = 0; // send first byte from buffer
    UART0_PROC_UDR = uart_send_char();
  }
}

/******************************************************/
// Enable/disable interrupts
/******************************************************/
void uart_interrupt_rx(byte enable)
{
  if (enable)
    UART0_PROC_UCSRB |= BV(UART0_PROC_RXCIE);
   else
    UART0_PROC_UCSRB &= ~BV(UART0_PROC_RXCIE);
}

void uart_interrupt_tx(byte enable)
{
  if (enable)
    UART0_PROC_UCSRB |= BV(UART0_PROC_TXCIE);
   else
    UART0_PROC_UCSRB &= ~BV(UART0_PROC_TXCIE);
}


/******************************
**         Interrupt         **
******************************/

/******************************************************/
//
/******************************************************/
ISR(UART0_PROC_RX_vect)
{
  byte tmpDat;
  byte tmpStatus;
  
  uart0_rx_timeout = UART0_TIMEOUT;
  tmpStatus = UART0_PROC_UCSRA;
  uart0_status |= tmpStatus;
  tmpDat = UART0_PROC_UDR;
  uart_receive_char(tmpDat);
}

/******************************************************/
//
/******************************************************/
ISR(UART0_PROC_TX_vect)
{
  byte tmpDat;

  // ???
  if (uart0_flags.txing == FALSE) return;

  // is next data in buffer?
  if (uart0_buf_tx_ptr > 8) {
    // whole buffer was sended
    uart0_flags.txing = FALSE;
    // if whole packed was send, wait for response
    //uart0_flags.wait_tx = TRUE;
    UART0_TX_DIS;   // rx mode
    return;
  } else {
    // send next byte
    tmpDat = uart_send_char();
    UART0_PROC_UDR = tmpDat;
  }
}


/******************************************************/
// External Function
/******************************************************/

//----------------------------------------------------------
// Initialization
void uart0_init(void)
{
  // UART port

  UART0_PROC_UBRRL = (F_CPU / (16UL * UART0_DEFAULT_BAUD)) - 1;

  UART0_PROC_UCSRB |= BV(UART0_PROC_TXEN) | BV(UART0_PROC_RXEN); /* tx/rx enable */

  uart_interrupt_rx(TRUE);
  uart_interrupt_tx(TRUE);
  uart0_status = 0;
}

//----------------------------------------------------------
// process internal logic
void uart0_process(void)
{
  byte i;
  byte iptr;
  byte sum;
  byte *ptr;

  // pøedává zpravy na odvysílání z paketového do lineárního bufferu
  if (!uart_pac_tx_empty()) {
    // jsou data k odesláni ?
    if ((!uart0_flags.txing)) {
      // nevysíláme ?
      // zaèneme vysílat další zprávu
      iptr = (uart0_buf_pac_tx_ptr_b+1) & UART0_BUFFER_PACKET_SIZE_MAX;
      ptr = (byte *) &(uart0_buf_pac_tx[iptr].b[0]);
      for (i=0; i<9; i++) {
        uart0_buf_tx[i] = *ptr;
        ptr++;
      }
      uart0_buf_pac_tx_ptr_b = iptr;
      uart0_buf_tx_ptr = 0;
      uart_send();
    }
  }

  // kontroluje pøijatá data
  if ((!uart0_flags.data_received) && (uart_rx_size() > 8)) {
    // máme alespoò 9 bytù dat a nejsou nezpracovaná data?


    // zjístíme adresu volného místa v paketovém pøijímacm bufferu
    iptr = (uart0_buf_pac_rx_ptr_e+1) & UART0_BUFFER_PACKET_SIZE_MAX;
    ptr = (byte *) &(uart0_buf_pac_rx[iptr].b[0]);

    // pøedáme do paketového pøijímacího bufferu
    for(i=0; i<9; i++) {
      *ptr = uart_get_char();
      ptr++;
      //uart0_buf_pac_rx[0].b[i] = uart_get_char(); 
    }
    
    // odpovídá kontrolní souèet?
    sum = 0;
    for(i=0; i<8; i++) {
      sum += uart0_buf_pac_rx[iptr].b[i];
    }
    if (sum == uart0_buf_pac_rx[iptr].b[8]) {
      // souèet v poøádku
      uart0_flags.data_received = TRUE; 
      uart0_buf_pac_rx_ptr_e = iptr;

     } else {
      uart0_flags.data_receive_error = TRUE;
    }
  }

}

//----------------------------------------------------------
// timer function
void uart0_ISR_timer(void)
{
  static byte uart0_rx_timeout_flag = 0;
  
  // smazání náhodnì pøijatých dat
  if (uart0_rx_timeout > 0) {
    uart0_rx_timeout--;
    uart0_rx_timeout_flag = FALSE;
    } else {
    if (!uart0_rx_timeout_flag) {
      uart0_buf_rx_ptr_b = uart0_buf_rx_ptr_e;
      uart0_rx_timeout_flag = TRUE;
    }
  }
}

//----------------------------------------------------------
// get pointer to received data
// must be called uart0_get_data_end() at end of handling data
byte * uart0_get_data_begin(void)
{
  byte iptr;
  
  iptr = (uart0_buf_pac_rx_ptr_b+1) & UART0_BUFFER_PACKET_SIZE_MAX;
  return (byte *) &uart0_buf_pac_rx[iptr].b[0];
}

//----------------------------------------------------------
// clear data_received flag and return number of pending packets
// must be called after uart0_get_data_begin()
byte uart0_get_data_end(void)
{
  byte i;
  
  i = (uart0_buf_pac_rx_ptr_b+1) & UART0_BUFFER_PACKET_SIZE_MAX;
  uart0_buf_pac_rx_ptr_b = i;
  
  i = uart_pac_rx_size();
  if (i == 0) {
    uart0_flags.data_received = FALSE;
  }
  return i;
}


//----------------------------------------------------------
// send packet pointed by dataptr
void uart0_put_data(byte * dataptr)
{
  byte i;
  byte iptr;
  byte sum;

  iptr = (uart0_buf_pac_tx_ptr_e+1) & UART0_BUFFER_PACKET_SIZE_MAX;

  sum = 0;
  // copy data with sum
  for(i=0; i<8; i++) {
    uart0_buf_pac_tx[iptr].b[i] = *dataptr;
    sum += *dataptr;
    dataptr++;
  }
  uart0_buf_pac_tx[iptr].n.sum = sum; // save calculated sum

  uart0_buf_pac_tx_ptr_e = iptr;
}


//----------------------------------------------------------
// Set from default baud rates
#define MACRO_BAUDRATE(BAUDRATE) (UART0_PROC_UBRRL = (F_CPU / (16UL * BAUDRATE)) - 1)
void uart0_set_baud(byte baud)
{
  // UART port
  switch (baud) {
    case 0:  
      MACRO_BAUDRATE(9600);
      break;
    case 1:  
      MACRO_BAUDRATE(14400);
      break;
    case 2:  
      MACRO_BAUDRATE(19200);
      break;
    case 3:  
      MACRO_BAUDRATE(28800);
      break;
    case 4:  
      MACRO_BAUDRATE(38400);
      break;
    case 5:  
      MACRO_BAUDRATE(57600);
      break;
    case 6:  
      MACRO_BAUDRATE(76800);
      break;
    case 7:  
      MACRO_BAUDRATE(115200);
      break;
    case 8:  
      MACRO_BAUDRATE(230400);
      break;
    case 9:  
      MACRO_BAUDRATE(250000);
      break;
    case 10:  
      MACRO_BAUDRATE(500000);
      break;
    default:  
      MACRO_BAUDRATE(19200);
      break;
  }
  return;
}

