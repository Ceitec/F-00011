
//  * DS18B20.h
//  *
//  * Created: 4.5.2016 9:47:20
//  *  Author: atom2
  


#ifndef DS18B20_H_
#define DS18B20_H_



#define THERM_PORT PORTD
#define THERM_DDR DDRD
#define THERM_PIN PIND
#define THERM_IO D2
#define THERM_INPUT()		cbi(THERM_DDR,THERM_IO) // make pin an input
#define THERM_OUTPUT()		sbi(THERM_DDR,THERM_IO) // make pin an output
#define THERM_LOW()			cbi(THERM_PORT,THERM_IO) // take (output) pin low
#define THERM_HIGH()		sbi(THERM_PORT,THERM_IO) // take (output) pin high
#define THERM_READ()		rbi(THERM_PIN,THERM_IO) // get (input) pin value
#define THERM_CONVERTTEMP	0x44
#define THERM_READSCRATCH	0xBE
#define THERM_WRITESCRATCH	0x4E
#define THERM_COPYSCRATCH	0x48
#define THERM_READPOWER		0xB4
#define THERM_SEARCHROM		0xF0
#define THERM_READROM		0x33
#define THERM_MATCHROM		0x55
#define THERM_SKIPROM		0xCC
#define THERM_ALARMSEARCH	0xEC


byte therm_Reset(void);
void therm_WriteBit(byte bit);
byte therm_ReadBit();
void therm_WriteByte(byte data);
byte therm_ReadByte();
void RomReaderProgram();
void therm_MatchRom(byte rom[]);
void therm_ReadTempRaw(byte id[], byte *t0, byte *t1);
void therm_ReadTempC(byte id[], int *whole, int *decimal);
void therm_ReadTempCTry(byte id[], int *whole, int *decimal, int *cold);
void therm_ReadTempDS18S20(byte id[], int *whole, int *decimal);



#endif /* DS18B20_H_ */

