/*
 * Bootloader-Slave.c
 *
 * Created: 27.4.2016 10:13:28
 * Author : atom2
 */ 

#include <avr/io.h>

#include "inc/Define_Setup.h"
#define	F_CPU	14745600UL
#include "inc/AllInit.h"
#include "inc/common_defs.h"
#include "inc/RS232.h"
#define	SIGRD 5
#include <avr/boot.h>
#include <avr/fuse.h>

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "inc/Bootloader_def.h"

#include "inc/defines.h"
#include "inc/timer.h"
#include "inc/uart_types.h"
#include "inc/uart_tri_0.h"
#include "inc/Tribus_types.h"
#include "inc/Tribus.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "inc/CRC_interface.h"

/************************************************************************/
/* Programovací pojistky se musí nastavit								*/
/************************************************************************/
/*							Atmega644P                                  */
/************************************************************************/
/*
BOOTRSZ = 0
BOOTSZ = "00"
LOW FUSE BITS = 0xD7
HIGH FUSE BITS = 0xD0
EXTENDED FUSE BITS = 0xFC
*/
/************************************************************************/
/*							Atmega32                                    */
/************************************************************************/
/*
BOOTRSZ = 0
BOOTSZ = "00"
LOW FUSE BITS = 0x1F
HIGH FUSE BITS = 0xC0
*/
/************************************************************************/



/************************************************************************/
/* Definování Bufferu, do kterého se vloží pøijatá data                 */
/************************************************************************/

static uint8_t BufferFlash[SPM_PAGESIZE];
static uint16_t	cnt=0;
static uint8_t	BootStatus=0;
uint16_t Address=0;
uint16_t Aktualni=0;
uint16_t Predchozi=0;
uint8_t	EndCom=0;
long int Signature;
uint8_t NowEeprom=0, NowFlash=0;

/************************************************************************/
/*                         FUNKCE                                       */
/************************************************************************/


// Skoèení na adresu 0x0000
void jumpaddress(void)
{
	asm("rjmp 0x0000");
}

// Vymazání celé aplikaèní pamìti
void ChipErase(void)
{
	uint16_t	address = 0x0000, konec = 0x0000;
	konec = END_APP_ADDRESS_BYTES;
	// Maže od adresy 0x0000 po koneènou adresu Bootloaderu
	while (address < konec)
	{
		// Vymaže pøíslušnou adresu
		
		boot_page_erase(address);
		boot_spm_busy_wait();
		address += SPM_PAGESIZE;
	}
}

// Vymazání EEprom
void EepromErase(void)
{
	uint16_t	address = 0x0000, konec = 0x0000;
	konec = END_EEPROM_ADDRESS;
	// Maže od adresy 0x0000 po koneènou adresu Bootloaderu
	while (address < konec)
	{
		// Vymaže pøíslušnou adresu
		if(eeprom_read_byte(address) != 0xFF)
		{
			eeprom_update_byte(address, 0xFF);
		}
		address++;
	}
}

// Zapsání Lock bitù
void WriteLockBits(void)
{
	// V této verzi není zápis Lock bitù povolen, kvùli zamknutí MCU...
	//boot_lock_bits_set(RS232_Receive_Char());
	//boot_spm_busy_wait();
}

// Zápis po pages do pamìti Flash
void WriteFlashPages(uint16_t address, uint8_t *Buffer)
{
	uint16_t i;
	uint16_t Data=0;
	for (i = 0; i < SPM_PAGESIZE; i += 2)
	{
		Data = *Buffer++;
		Data |= *Buffer++ << 8;
		// Plní Page buffer (256 velikost u 644p)
		boot_page_fill (address + i, Data);
	}

	boot_page_write (address);     //Naplní buffer
	boot_spm_busy_wait();       // Èeká dokud se neuvolní
}

// Zápis po pages do pamìti EEPROM
void WriteEepromPages(uint16_t address, uint8_t *Buffer)
{
	uint16_t cnt=0;
	for (cnt = 0; cnt < PAGE_SIZE_EEPROM; cnt++)
	{
		eeprom_update_byte(address++, *Buffer++);
	}
}

// Ètení pamìti Flash
void ReadFlashPages(uint16_t address)
{
	for (uint8_t x=0; x < 4; x++)
	{
		TB_SendAck(ACK, pgm_read_byte(address + x));	
	}
}

// Ètení pamìti EEPROM
void ReadEepromPages(uint16_t address)
{
	for (uint8_t x=0; x < 4; x++)
	{
		TB_SendAck(ACK, eeprom_read_byte(address + x));
	}
}

void FillBufferData(uint16_t Address)
{
	BufferFlash[(uint8_t)(Address & ADDRESS_MASK_LSB)] = TB_bufIn[TB_BUF_DATA1];
	BufferFlash[(uint8_t)(Address & ADDRESS_MASK_LSB) + 1] = TB_bufIn[TB_BUF_DATA2];
	BufferFlash[(uint8_t)(Address & ADDRESS_MASK_LSB) + 2] = TB_bufIn[TB_BUF_DATA3];
	BufferFlash[(uint8_t)(Address & ADDRESS_MASK_LSB) + 3] = TB_bufIn[TB_BUF_DATA4];
}




// Init pro Tribus
volatile byte timer0_flag = 0; // T = 10ms
byte led_timer = 0;



void send_data(void)
{
	uart0_put_data((byte *) &TB_bufOut);
}

//----------------------------------------------------------
ISR(TIMER1_CAPT_vect) {
	// T = 10ms
	
	timer0_flag = TRUE;
	
}

//----------------------------------------------------------
void process_timer_100Hz(void)
{
	if (timer0_flag) { // T = 10ms
		timer0_flag = FALSE;
		uart0_ISR_timer();
		if (led_timer > 0) {
			led_timer--;
			if (led_timer == 0) {
				//PORTA ^= (1 << PINA6);
			}
		}
	}
}



void try_receive_data(void)
{
	byte i;
	byte *ptr;
	
	if (uart0_flags.data_received)
	{
		ptr = uart0_get_data_begin();
		for (i=0; i<9; i++)
		{
			TB_bufIn[i] = *ptr;
			ptr++;
		}
		uart0_get_data_end();
		uart0_flags.data_received = FALSE;
		if (TB_Read() == 0)
		{
			switch (TB_Decode())
			{
				case ENTER_BOOTLOADER:
					TB_SendAck(ACK, ENTER_BOOTLOADER);
					break;
			}
		}
	}
}



int main(void)
{
	
    /* Replace with your application code */
	cli();
	
	
	//Nastavení Systemového enable pro RS485 pro UART0
	DDRD |= (1 << DDD1) | (1 << DDD2);
	
	DDRA |= (1 << DDA7) | (1 << DDA6) | (1 << DDA5);
	
	//timer_init();
	
	uart0_init();
	TB_Callback_setBaud = &uart0_set_baud;
	TB_Callback_TX = &send_data;
	TB_Init((void*) 0x10); // addr in eeprom with settings
	
	sei();
	
    while (1) 
    {
		//process_timer_100Hz();
		uart0_process();
		try_receive_data();
		PORTA |= (1 << PA7) | (1 << PA6) | (1 << PA5);
		/*if(cnt < 25)
		{
			cnt++;
		}
		else
		{
			if (!BootStatus)
			{
				cnt = 0;
				jumpaddress();
			}
		}	*/
    }
}

