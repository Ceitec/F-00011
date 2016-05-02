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
uint8_t	EndComFlash=0, EndComEeprom=0;
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
	for (cnt = 0; cnt < SPM_PAGESIZE; cnt++)
	{
		eeprom_update_byte(address++, *Buffer++);
	}
}

// Ètení pamìti Flash
void ReadFlashPages(uint16_t address)
{
	for (uint8_t i=0; i<4; i++)
	{
		
	}
	uint32_t Data=0;
	Data = pgm_read_byte(address) << 8;
	Data |= pgm_read_byte(address + 1) << 8;
	Data |= pgm_read_byte(address + 2) << 8;
	Data |= pgm_read_byte(address + 3);
	TB_SendAck(100, Data);	
}

// Ètení pamìti EEPROM
void ReadEepromPages(uint16_t address)
{
	TB_SendAck(100, eeprom_read_dword(address));
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
	if(timer0_flag)
	{ // T = 1ms
		timer0_flag = FALSE;
		uart0_ISR_timer();
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
					BootStatus = 1;
					TB_SendAck(100, ENTER_BOOTLOADER);
					break;
				case READ_LOCK_BITS:
					TB_SendAck(100, boot_lock_fuse_bits_get(GET_LOCK_BITS));
					break;
				case CHIP_ERASE_FLASH:
					ChipErase();
					TB_SendAck(100, CHIP_ERASE_FLASH);
					break;
				case CHIP_ERASE_EEPROM:
					EepromErase();
					TB_SendAck(100, CHIP_ERASE_EEPROM);
					break;
				case CHIP_ERASE_ALL:
					ChipErase();
					EepromErase();
					TB_SendAck(100, CHIP_ERASE_ALL);
					break;
				case WRITE_LOCK_BITS:
					TB_SendAck(102, WRITE_LOCK_BITS);
					break;
				case READ_LOW_FUSE:
					TB_SendAck(100, boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS));
					break;
				case READ_HIGH_FUSE:
					TB_SendAck(100, boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS));
					break;
				case READ_EXTENDED_FUSE:
					TB_SendAck(102, boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS));
					break;
				case READ_SIGNATURE:
					Signature = 0x1E9502;
// 					Signature |= 0x95 << 8;
// 					Signature |= 0x02;
					TB_SendAck(100, Signature);
					break;
				case READ_SOFTWARE_VERSION:
					TB_SendAck(100, SOFTWARE_IDENTIFIER);
					break;
				case READ_BOOTLOADER_VERSION:
					TB_SendAck(100, BOOTLOADER_VERSION);
					break;
				case VERIFY_FLASH:
					Address = TB_bufIn[TB_BUF_TYPE] << 8;
					Address |= TB_bufIn[TB_BUF_MOTOR];
					VerifyFlash(Address);
					break;
				case WRITE_FLASH:
					Address = TB_bufIn[TB_BUF_TYPE] << 8;
					Address |= TB_bufIn[TB_BUF_MOTOR];
					Aktualni = (Address & ADDRESS_MASK_MSB);
					if (Address < (START_BOOT_ADDRESS_BYTES - 4))
					{
						if (Aktualni == Predchozi)
						{
							FillBufferData(Address);
							EndComFlash = 1;
							TB_SendAck(100, WRITE_FLASH);
						} 
						else
						{
							if (EndComFlash)
							{
								WriteFlashPages(Predchozi, BufferFlash);
								memset(BufferFlash, 0xFF, SPM_PAGESIZE);
								FillBufferData(Address);
								TB_SendAck(100, Address);
								Predchozi = Address;
							}
							else
							{
								memset(BufferFlash, 0xFF, SPM_PAGESIZE);
								FillBufferData(Address);
								TB_SendAck(100, WRITE_FLASH);
							}
						}
						NowFlash = 1;
					}
					else
					{
						TB_SendAck(102, Address);	
					}
					break;
				case WRITE_EEPROM:
					Address = TB_bufIn[TB_BUF_TYPE] << 8;
					Address |= TB_bufIn[TB_BUF_MOTOR];
					Aktualni = (Address & ADDRESS_MASK_MSB);
					if (Address < (START_EEPROM_ADDRESS - 4))
					{
						if (Aktualni == Predchozi)
						{
							FillBufferData(Address);
							EndComEeprom = 1;
							TB_SendAck(100, WRITE_EEPROM);
						}
						else
						{
							if (EndComEeprom)
							{
								WriteEepromPages(Predchozi, BufferFlash);
								memset(BufferFlash, 0xFF, SPM_PAGESIZE);
								FillBufferData(Address);
								TB_SendAck(100, Address);
								Predchozi = Address;
							}
							else
							{
								memset(BufferFlash, 0xFF, SPM_PAGESIZE);
								FillBufferData(Address);
								TB_SendAck(100, WRITE_EEPROM);
							}
						}
						NowEeprom = 1;
					}
					else
					{
						TB_SendAck(102, Address);
					}
					break;
				case END_WRITE_ALL:
					if (NowFlash)
					{
						WriteFlashPages(Predchozi, BufferFlash);
						EndComFlash=0;
						TB_SendAck(100, END_WRITE_ALL);
					}
					else if(NowEeprom)
					{
						WriteEepromPages(Predchozi, BufferFlash);
						EndComEeprom = 0;
						TB_SendAck(100, END_WRITE_ALL);
					}
					else
					{
						TB_SendAck(102, 0);
					}
					Predchozi = 0;
					Aktualni = 0;
					break;
				case READ_FLASH:
					Address = TB_bufIn[TB_BUF_TYPE] << 8;
					Address |= TB_bufIn[TB_BUF_MOTOR];
					TB_SendAck(100, Address);
					if (Address < (START_BOOT_ADDRESS_BYTES - 4))
					{
						uint32_t Data=0;
						Data = (((uint32_t)pgm_read_byte(Address)) << 24);
						TB_SendAck(100, Data);
						Data = (pgm_read_byte(Address) << 8);
						TB_SendAck(100, Data);
						Data |= pgm_read_byte(Address + 1) << 16;
						TB_SendAck(100, Data);
						Data |= pgm_read_byte(Address + 2) << 8;
						TB_SendAck(100, Data);
						Data |= pgm_read_byte(Address + 3);
						TB_SendAck(100, Data);
					}
					else
					{
						TB_SendAck(102, 0);	
					}
					break;
				case READ_EEPROM:
					Address = TB_bufIn[TB_BUF_TYPE] << 8;
					Address |= TB_bufIn[TB_BUF_MOTOR];
					if (Address < (START_EEPROM_ADDRESS - 4))
					{
						uint32_t Data=0;
//						Data = pgm_read_byte(Address) << 8;
// 						Data |= pgm_read_byte(address + 1) << 8;
// 						Data |= pgm_read_byte(address + 2) << 8;
// 						Data |= pgm_read_byte(address + 3);
						TB_SendAck(100, Data);
						//ReadEepromPages(Address);
					}
					else
					{
						TB_SendAck(102, 0);
					}
					break;
				case EXIT_BOOTLOADER:
					TB_SendAck(100, EXIT_BOOTLOADER);
					jumpaddress();
					break;
			}
		}
	}
}

void Move_interrupts(void)
{
	/* Enable change of interrupt vectors */
	GICR = (1<<IVCE);
	/* Move interrupts to boot Flash section */
	GICR = (1<<IVSEL);
}

void Bootloader_Init(void)
{
	memset(BufferFlash, 0xFF, SPM_PAGESIZE);
}

int main(void)
{
	// Interrupty pøesunuty na jinou adresu pro Bootloader.
	Move_interrupts();
    /* Replace with your application code */
	cli();

	//Nastavení Systemového enable pro RS485 pro UART0
	DDRD |= (1 << DDD1) | (1 << DDD2);

	timer_init();
	
	uart0_init();
	TB_Callback_setBaud = &uart0_set_baud;
	TB_Callback_TX = &send_data;
	TB_Init((void*) 0x10); // addr in eeprom with settings
	Bootloader_Init();
	sei();
		
    while (1) 
    {
		process_timer_100Hz();
		uart0_process();
		try_receive_data();
	
		// 250 ms - V Bootloader režimu.
		/*if(cnt < 250)
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
		}*/	
    }
}

