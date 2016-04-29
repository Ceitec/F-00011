/*
 *
 * timer initialization and operation
 *
 */

#include <avr/io.h>
#include "common_defs.h"
#include "timer.h"

//----------------------------------------------------
void timer_init(void)
{
  // Xtal = 14 745 600 Hz

  // * Timer 0 - unused
  //TCCR0 = 0; // stopped


  // * Timer 1 - system timer and 2× PWM
  // 14745600 / 256 / 576 = 100 Hz
  //   Xtal  /presca/ TOP
  
  OCR1A = 0;
  OCR1B = 0;
  ICR1 = 575; 
  TCCR1A = BV(WGM11) | BV(COM1A1) | BV(COM1B1); // Fast PWM
  TCCR1B = BV(WGM12) | BV(WGM13) | 4; // Fast PWM + presca = 1024
  TIMSK |= BV(TICIE1); // capt int enabled
	
  // * Timer 2 - unused
  //TCCR2 = 0; // stopped
}

//----------------------------------------------------

//----------------------------------------------------
