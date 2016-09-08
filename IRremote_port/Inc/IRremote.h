//******************************************************************************
// IRremote
// Version 2.0.1 June, 2015
// Copyright 2009 Ken Shirriff
// For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
// Edited by Mitra to add new controller SANYO
//
// Interrupt code based on NECIRrcv by Joe Knapp
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
// Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
//
// JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
// LG added by Darryl Smith (based on the JVC protocol)
// Whynter A/C ARC-110WD added by Francesco Meschia
//******************************************************************************

#ifndef IRremote_h
#define IRremote_h

#include "IRremoteInt.h"
#include <stdint.h>
//------------------------------------------------------------------------------
// Supported IR protocols
// Each protocol you include costs memory and, during decode, costs time
// Disable (set to 0) all the protocols you do not need/want!
//
#define IR_DECODE_RC5           1
#define IR_SEND_RC5             1

#define IR_DECODE_RC6           1
#define IR_SEND_RC6             1

#define IR_DECODE_SONY          1
#define IR_SEND_SONY            1

#define IR_DECODE_SANYO         1

//------------------------------------------------------------------------------
// An enumerated list of all supported formats
// You do NOT need to remove entries from this list when disabling protocols!
//
typedef enum
{
	UNKNOWN      = -1,
	UNUSED       =  0,
	RC5,
	RC6,
	SONY,
	SANYO
} ir_decode_type_t;

//------------------------------------------------------------------------------
// Set DEBUG to 1 for lots of lovely debug output
//
#define IR_DEBUG  0

//------------------------------------------------------------------------------
// Debug directives
//
#if IR_DEBUG
#	define IR_DBG_PRINT(...)    printf(__VA_ARGS__)
#	define IR_DBG_PRINTLN(...)  printf(__VA_ARGS__); printf("\n")
#else
#	define IR_DBG_PRINT(...)
#	define IR_DBG_PRINTLN(...)
#endif

//------------------------------------------------------------------------------
// Mark & Space matching functions
//
int  IR_MATCH       (int measured, int desired) ;
int  IR_MATCH_MARK  (int measured_ticks, int desired_us) ;
int  IR_MATCH_SPACE (int measured_ticks, int desired_us) ;

//------------------------------------------------------------------------------
// Results returned from the decoder
//
typedef struct
{
	ir_decode_type_t       decode_type;  // UNKNOWN, NEC, SONY, RC5, ...
	unsigned int           address;      // Used by Panasonic & Sharp [16-bits]
	unsigned long          value;        // Decoded value [max 32-bits]
	int                    bits;         // Number of bits in decoded value
	volatile unsigned int  *rawbuf;      // Raw intervals in 50uS ticks
	int                    rawlen;       // Number of records in rawbuf
	int                    overflow;     // true iff IR raw code too long
} ir_decode_results;

//------------------------------------------------------------------------------
// Decoded value for NEC when a repeat code is received
//
#define IR_REPEAT 0xFFFFFFFF

//------------------------------------------------------------------------------
// Functions for receiving IR
//

extern void IRrecv_init1 (int recvpin);
extern void IRrecv_init2 (int recvpin, int blinkpin);

extern int      IRrecv_decode     (ir_decode_results *results);
extern void     IRrecv_enableIRIn (void);
extern uint8_t  IRrecv_isIdle     (void);
extern void     IRrecv_resume     (void);

long  IRrecv_decodeHash (ir_decode_results *results) ;
int   IRrecv_compare    (unsigned int oldval, unsigned int newval);

//......................................................................
#if (IR_DECODE_RC5 || IR_DECODE_RC6)
	// This helper function is shared by RC5 and RC6
	int IRrecv_getRClevel (ir_decode_results *results, int *offset,  int *used,  int t1);
#endif

#if IR_DECODE_RC5
	extern uint8_t IRrecv_decodeRC5 (ir_decode_results *results);
#endif
#if IR_DECODE_RC6
	extern uint8_t IRrecv_decodeRC6 (ir_decode_results *results);
#endif

//......................................................................
#if IR_DECODE_SONY
	extern uint8_t IRrecv_decodeSony (ir_decode_results *results);
#endif

extern void IR_ISR (void);


//------------------------------------------------------------------------------
// Functions for sending IR
//

extern void IRsend_custom_delay_usec (unsigned long uSecs);
extern void IRsend_enableIROut 	     (uint32_t khz);
extern void IRsend_mark        	     (unsigned int usec);
extern void IRsend_space       	     (unsigned int usec);
extern void IRsend_sendRaw     	     (const unsigned int buf[], unsigned int len, unsigned int hz);

//......................................................................
#if IR_SEND_RC5
extern void IRsend_sendRC5 (unsigned long data, int nbits);
#endif

#if IR_SEND_RC6
extern void IRsend_sendRC6 (unsigned long data, int nbits);
#endif
//......................................................................
#if IR_SEND_SONY
extern void IRsend_sendSony (unsigned long data, int nbits);
#endif
//......................................................................


extern void Error_Handler(void);

#endif
