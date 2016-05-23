
 /*
 * DS18B20.c
 *
 * Created: 4.5.2016 9:47:31
 *  Author: atom2
 */ 

#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "common_defs.h"
#include "DS18B20.h"
#include "Arduino328p.h"


byte therm_Reset()
{
	byte i;
	THERM_OUTPUT(); // set pin as output
	THERM_LOW(); // pull pin low for 480uS
	_delay_us(480);
	THERM_INPUT(); // set pin as input
	_delay_us(60); // wait for 60uS
	i = THERM_READ(); // get pin value
	_delay_us(420); // wait for rest of 480uS period
	return i;
}

void therm_WriteBit(byte bit)
{
	THERM_OUTPUT(); // set pin as output
	THERM_LOW(); // pull pin low for 1uS
	_delay_us(1);
	if (bit) THERM_INPUT(); // to write 1, float pin
	_delay_us(60);
	THERM_INPUT(); // wait 60uS & release pin
}

byte therm_ReadBit()
{
	byte bit=0;
	THERM_OUTPUT(); // set pin as output
	THERM_LOW(); // pull pin low for 1uS
	_delay_us(1);
	THERM_INPUT(); // release pin & wait 14 uS
	_delay_us(14);
	if (THERM_READ()) bit=1; // read pin value
	_delay_us(45); // wait rest of 60uS period
	return bit;
}

void therm_WriteByte(byte data)
{
	byte i=8;
	while(i--) // for 8 bits:
	{
		therm_WriteBit(data&1); // send least significant bit
		data >>= 1; // shift all bits right
	}
}

byte therm_ReadByte()
{
	byte i=8, data=0;
	while(i--) // for 8 bits:
	{
		data >>= 1; // shift all bits right
		data |= (therm_ReadBit()<<7); // get next bit (LSB first)
	}
	return data;
}

void therm_MatchRom(byte rom[])
{
	therm_WriteByte(THERM_MATCHROM);
	for (byte i=0;i<8;i++)
	therm_WriteByte(rom[i]);
}

void therm_ReadTempRaw(byte id[], byte *t0, byte *t1)
// Returns the two temperature bytes from the scratchpad
{
	therm_Reset(); // skip ROM & start temp conversion
	if (id) therm_MatchRom(id);
	else therm_WriteByte(THERM_SKIPROM);
	therm_WriteByte(THERM_CONVERTTEMP);
	while (!therm_ReadBit()); // wait until conversion completed

	therm_Reset(); // read first two bytes from scratchpad
	if (id) therm_MatchRom(id);
	else therm_WriteByte(THERM_SKIPROM);
	therm_WriteByte(THERM_READSCRATCH);
	*t0 = therm_ReadByte(); // first byte
	*t1 = therm_ReadByte(); // second byte
}

void therm_ReadTempC(byte id[], int *whole, int *decimal)
// returns temperature in Celsius as WW.DDDD, where W=whole & D=decimal
{
	byte t0,t1;
	therm_ReadTempRaw(id,&t0,&t1); // get temperature values
	*whole = (t1 & 0x07) << 4; // grab lower 3 bits of t1
	*whole |= t0 >> 4; // and upper 4 bits of t0
	*decimal = t0 & 0x0F; // decimals in lower 4 bits of t0
	*decimal *= 625; // conversion factor for 12-bit resolution
}

void therm_ReadTempCTry(byte id[], int *whole, int *decimal, int *cold)
// returns temperature in Celsius as WW.DDDD, where W=whole & D=decimal
{
	byte t0,t1;
	therm_ReadTempRaw(id,&t0,&t1); // get temperature values
	*whole = (t1 & 0x07) << 4; // grab lower 3 bits of t1
	*whole |= t0 >> 4; // and upper 4 bits of t0
	*decimal = ((t0 & 0x0C) >> 2) & 0x03; // decimals in lower 4 bits of t0
	*decimal *= 25; // conversion factor for 12-bit resolution
	*cold = (t1 >> 7) & 0x01;
}

void therm_ReadTempDS18S20(byte id[], int *whole, int *decimal)
// returns temperature in Celsius as WW.DDDD, where W=whole & D=decimal
{
	byte t0,t1;
	therm_ReadTempRaw(id,&t0,&t1); // get temperature values
	*whole = (t0 >> 1) & 0x7F;
	//*whole = (t0 & 0xFE) >> 1; // grab lower 3 bits of t1
	//*whole |= t0 >> 4; // and upper 4 bits of t0
	*decimal = t0 & 0x01; // decimals in lower 4 bits of t0
	*decimal *= 5; // conversion factor for 12-bit resolution
}

/*
void RomReaderProgram()
// Read the ID of the attached Dallas 18B20 device
// Note: only ONE device should be on the bus.
{
	RS232_Transmit_String("ID (ROM) Reader:");
	// write 64-bit ROM code on first LCD line
	therm_Reset();
	therm_WriteByte(THERM_READROM);
	for (byte i=0; i<8; i++)
	{
		byte data = therm_ReadByte();
		rom0[i] = data;
		sprintf(Vystup, "%i", rom0[i]);   // Spr?vn?
		RS232_Transmit_String(Vystup);
		RS232_Transmit_String(" ");
		memset(Vystup, 0, 3);
	}
	RS232_Transmit_String_CRLF(".");
}
*/