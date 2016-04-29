#ifndef __TRIBUS_H
#define __TRIBUS_H

#define TRI_LEN (9) 

// ttype defines
struct TB_IO {
  byte b0 : 1;
  byte b1 : 1;
  byte b2 : 1;
  byte b3 : 1;
  byte b4 : 1;
  byte b5 : 1;
  byte b6 : 1;
  byte b7 : 1;
};

struct TB_PARAM {
  unsigned long target_position;
  unsigned long actual_position;
  unsigned long speed;
  unsigned long acceleration;
  unsigned long current;
  unsigned long current_hold;
//  unsigned word position reached : 1; // 8
  byte resolution; // 140
  unsigned long rfs_distance; // 196 (length output)
};

struct TB_GBPARAM {
  byte eemagic;
  byte baud;
  byte address;
  byte telegram_pause_time;
  byte host_address;

};

// shared variables
extern struct TB_PARAM TB_param;
extern struct TB_GBPARAM TB_gbparam;
extern struct TB_IO TB_inp;
extern struct TB_IO TB_out;

extern byte TB_bufOut[9];
extern byte TB_bufIn[9];
extern byte TB_AddrReply;
extern byte TB_AddrModule;
extern long int TB_Value;
extern byte TB_send_flag;

extern void (*TB_Callback_TX)(void); // callback for sending data
extern void (*TB_Callback_setBaud)(byte); // callback for switch uart speed

// init
void TB_Init(void * setting_in_eeprom);

// main
byte TB_Read(void);
byte TB_Decode(void);

// auxiliary
void TB_SendAck(byte status, long int value);
void TB_SendSerVzorku(byte addr, byte command, byte type, byte Type4, byte Type3, byte Type2, byte Type1, byte Type0);

void TB_SendAckOK(void);



#endif
