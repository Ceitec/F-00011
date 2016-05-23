/***********************************************
*         Trinamic  Bus  libraly               *
***********************************************/

#include "common_defs.h"
#include "defines.h"
#include <avr/eeprom.h>
//#include <stdint.h>
#include "Tribus_types.h"
#include "Tribus.h"
#include "Tribus_conf.h"
#include "AllInit.h"
#include "Bootloader_def.h"

/******************************************************/
//variables
struct TB_PARAM TB_param;
struct TB_GBPARAM TB_gbparam;
struct TB_IO TB_inp;
struct TB_IO TB_out;

byte TB_bufOut[9];
byte TB_bufIn[9];
byte TB_AddrReply;
byte TB_AddrModule;
void * addr_setting_in_eeprom;
int32_t TB_Value;
byte TB_send_flag = 0;
void (*TB_Callback_TX)(void) = NULL;
void (*TB_Callback_setBaud)(byte) = NULL;

unsigned char BUF_PC4 = 0x00;

/******************************************************/
// private functions
/******************************************************/
void TB_Send(void)
{
  if (TB_Callback_TX != NULL) TB_Callback_TX();
  //TB_send_flag = true;
}


/******************************************************/
void TB_calcSum(void)
{
  byte i, sum;
  sum = 0;
  for(i=0; i<8; i++) {
    sum += TB_bufOut[i];
  }
  TB_bufOut[TB_BUF_SUM] = sum;
}

/******************************************************/
// public functions
/******************************************************/
// initialize
void TB_Init(void * setting_in_eeprom)
{
  addr_setting_in_eeprom = setting_in_eeprom;
  //                 DST,   SRC, size
  eeprom_read_block((void *) &TB_gbparam, setting_in_eeprom, sizeof(struct TB_GBPARAM));
  if (TB_gbparam.eemagic != 66) {
    // not valid data in eeprom
    TB_gbparam.eemagic = 66;
    TB_gbparam.baud = 7;
    TB_gbparam.address = SC_MODUL;
    TB_gbparam.telegram_pause_time = 0;
    TB_gbparam.host_address = 2;
    // save default setting to eeprom
    eeprom_write_block((void *) &TB_gbparam, setting_in_eeprom, sizeof(struct TB_GBPARAM));
  }
  // ted mame funkèni konfiguraci naètenou

  // zvolíme správnou komunikaèní rychlost:
  if (TB_Callback_setBaud != NULL) TB_Callback_setBaud(TB_gbparam.baud);

  // poznaèíme si adresy
  TB_AddrReply = TB_gbparam.host_address;
  TB_AddrModule= TB_gbparam.address;
}

/******************************************************/
// try to read incoming data
// return 3 - invalid data;
// return 2 - another address
// return 1 - reserved (backward compatibility)
// return 0 - valid command
byte TB_Read(void)
{
  byte i;
  byte sum;

  // check address
  if (TB_bufIn[TB_BUF_ADDRESS] != TB_AddrModule) return 2;
  
  // check SUM byte
  sum = 0;
  for( i=0; i<8; i++) {
    sum += TB_bufIn[i];
  }
  if (sum != TB_bufIn[TB_BUF_SUM]) {
    TB_SendAck(1, 0); // wrong checksum
    return 3; // bad checksum
  }

  // we have valid data in TB_bufIn
  return 0;
}


/******************************************************/
// decode incoming command
// return = unhandled command number, 0=handled or unknown
byte TB_Decode(void)
{
	volatile byte b;
	TB_Value = (((int32_t) TB_bufIn[4]) << 24) |
			   (((int32_t) TB_bufIn[5]) << 16) |
               (((int32_t) TB_bufIn[6]) <<  8) |
               (((int32_t) TB_bufIn[7])      ) ;

	switch (TB_bufIn[TB_BUF_COMMAND])
	{
		case ENTER_BOOTLOADER:
			return ENTER_BOOTLOADER;
			break;
		case READ_LOCK_BITS:
			return READ_LOCK_BITS;
			break;
		case CHIP_ERASE_FLASH:
			return CHIP_ERASE_FLASH;
			break;
		case CHIP_ERASE_EEPROM:
			return CHIP_ERASE_EEPROM;
			break;
		case CHIP_ERASE_ALL:
			return CHIP_ERASE_ALL;
			break;
		case WRITE_LOCK_BITS:
			return WRITE_LOCK_BITS;
			break;
		case READ_LOW_FUSE:
			return READ_LOW_FUSE;
			break;
		case READ_HIGH_FUSE:
			return READ_HIGH_FUSE;
			break;
		case READ_EXTENDED_FUSE:
			return READ_EXTENDED_FUSE;
			break;
		case READ_SIGNATURE:
			return READ_SIGNATURE;
			break;
		case READ_SOFTWARE_VERSION:
			return READ_SOFTWARE_VERSION;
			break;
		case READ_BOOTLOADER_VERSION:
			return READ_BOOTLOADER_VERSION;
			break;
		case VERIFY_FLASH:
			return VERIFY_FLASH;
			break;			
		case WRITE_FLASH:
			return WRITE_FLASH;
			break;
		case WRITE_EEPROM:
			return WRITE_EEPROM;
			break;
		case END_WRITE_ALL:
			return END_WRITE_ALL;
			break;
		case READ_FLASH:
			return READ_FLASH;
			break;
		case READ_EEPROM:
			return READ_EEPROM;
			break;
		case CMD_SPM_PAGE_SIZE:
			return CMD_SPM_PAGE_SIZE;
			break;	
		case CMD_ALL_PAGE_SIZE:
			return CMD_ALL_PAGE_SIZE;
			break;
		case TB_CMD_SGP:
			if (TB_bufIn[TB_BUF_MOTOR] != 0)
			{
				TB_SendAck(TB_ERR_VALUE, 0); // invalid value
			}
			else
			{
				switch (TB_bufIn[TB_BUF_TYPE])
				{
					case TB_GBPARAM_EEMAGIC:
						if (TB_Value != TB_gbparam.eemagic)
						{
							TB_gbparam.eemagic = TB_Value;
							b = (void *) &(TB_gbparam.eemagic) - (void *) &(TB_gbparam);
							eeprom_update_byte(b+addr_setting_in_eeprom, TB_gbparam.eemagic);
						}
						TB_SendAck(TB_ERR_OK, 0);
						break;
					case TB_GBPARAM_BAUD:
						if (TB_Value != TB_gbparam.baud)
						{
							TB_gbparam.baud = TB_Value;
							b = (void *) &(TB_gbparam.baud) - (void *) &(TB_gbparam);
							eeprom_update_byte(b+addr_setting_in_eeprom, TB_gbparam.baud);
						}
						TB_SendAck(TB_ERR_OK, 0);
						break;
					case TB_GBPARAM_ADDRESS:
						if (TB_Value != TB_gbparam.address)
						{
							TB_gbparam.address = TB_Value;
					        b = (void *) &(TB_gbparam.address) - (void *) &(TB_gbparam);
							eeprom_update_byte(b+addr_setting_in_eeprom, TB_gbparam.address);
						}
						TB_SendAck(TB_ERR_OK, 0);
						break;
					case TB_GBPARAM_HOST_ADDR:
						if (TB_Value != TB_gbparam.host_address)
						{
							TB_gbparam.host_address = TB_Value;
							b = (void *) &(TB_gbparam.host_address) - (void *) &(TB_gbparam);
							eeprom_update_byte(b+addr_setting_in_eeprom, TB_gbparam.host_address);
						}
						TB_SendAck(TB_ERR_OK, 0);
						break;
					default:
						TB_SendAck(TB_ERR_VALUE, 0); // invalid value
						break;
				}
			}
			break;
		case TB_CMD_GGP:
			if (TB_bufIn[TB_BUF_MOTOR] != 0)
			{
				TB_SendAck(TB_ERR_VALUE, 0); // invalid value
			}
			else
			{
		        switch (TB_bufIn[TB_BUF_TYPE])
				{
					case TB_GBPARAM_BAUD:
						TB_SendAck(TB_ERR_OK, TB_gbparam.baud);
						break;
					case TB_GBPARAM_ADDRESS:
						TB_SendAck(TB_ERR_OK, TB_gbparam.address);
						break;
					case TB_GBPARAM_HOST_ADDR:
						TB_SendAck(TB_ERR_OK, TB_gbparam.host_address);
						break;
					case TB_GBPARAM_EEMAGIC:
						TB_SendAck(TB_ERR_OK, TB_gbparam.eemagic);
						break;
					default:
						TB_SendAck(TB_ERR_VALUE, 0); // invalid value
						break;
				}
			}
			break;
		case EXIT_BOOTLOADER:
			return EXIT_BOOTLOADER;
			break;
		default:
			TB_SendAck(TB_ERR_COMMAND, 0); // invalid command
			return 0;
	}
	return 0;
}

/******************************************************/
// set command to module
void TB_SetParam(byte addr, byte command, byte type, byte motor, long int value)
{
  TB_bufOut[0] = addr;
  TB_bufOut[1] = command;
  TB_bufOut[2] = type;
  TB_bufOut[3] = motor;
  TB_bufOut[4] = value >> 24;
  TB_bufOut[5] = value >> 16;
  TB_bufOut[6] = value >> 8;
  TB_bufOut[7] = value >> 0;
  TB_calcSum();
  TB_Send();
}

/******************************************************/
// send response from module
void TB_SendAck(byte status, long int value)
{
  TB_bufOut[0] = TB_AddrReply;
  TB_bufOut[1] = TB_AddrModule;
  TB_bufOut[2] = status;
  TB_bufOut[3] = TB_bufIn[TB_BUF_COMMAND]; //command;
  TB_bufOut[4] = value >> 24;
  TB_bufOut[5] = value >> 16;
  TB_bufOut[6] = value >> 8;
  TB_bufOut[7] = value >> 0;
  TB_calcSum();
  TB_Send();
}


/******************************************************/
// send OK response from module
inline void TB_SendAckOK(void)
{
  TB_SendAck(100, 0);
}