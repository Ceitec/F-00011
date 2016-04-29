#ifndef __COMMON_DEFS_DEFINED
#define __COMMON_DEFS_DEFINED

#ifndef __BYTE_DEFINED
#define __BYTE_DEFINED
typedef unsigned char byte;
#endif

#ifndef __WORD_DEFINED
#define __WORD_DEFINED
#ifdef __BORLANDC__
typedef unsigned short word ;
#else
typedef unsigned int word ;
#endif
#endif

#ifndef __DWORD_DEFINED
#define __DWORD_DEFINED
#ifdef __BORLANDC__
typedef unsigned int dword;
#else
typedef unsigned long dword;
#endif
#endif

#define BV 	_BV

// global AVRLIB defines
//#include <avrlibdefs.h>
// global AVRLIB types definitions
//#include <avrlibtypes.h>

#define u08 byte
#define u16 word

#define 	sbi(port, bit)   (port) |= (1 << (bit))
#define 	cbi(port, bit)   (port) &= ~(1 << (bit))

#define TRUE 1
#define FALSE 0

#endif
