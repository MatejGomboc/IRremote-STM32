//******************************************************************************
// Code based on https://github.com/z3t0/Arduino-IRremote !
//
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
//
#define IR_DECODE_SONY          1
#define IR_SEND_SONY            1

//------------------------------------------------------------------------------
// An enumerated list of all supported formats
// You do NOT need to remove entries from this list when disabling protocols!
//
typedef enum
{
	UNKNOWN      = -1,
	UNUSED       =  0,
	SONY
} ir_decode_type_t;

//------------------------------------------------------------------------------
// Set IR_DEBUG to 1 for debug output
//
#define IR_DEBUG  0

//------------------------------------------------------------------------------
// Debug directives
//
#if IR_DEBUG
#	define IR_DBG_PRINT_INT(integer)	printf("%d", integer)
#	define IR_DBG_PRINT(string)    		printf(string)
#	define IR_DBG_PRINTLN(string)  		{printf(string); printf("\n");}
#else
#	define IR_DBG_PRINT_INT(...)
#	define IR_DBG_PRINT(...)
#	define IR_DBG_PRINTLN(...)
#endif

//------------------------------------------------------------------------------
// Mark & Space matching functions
//
int  IR_MATCH       (int measured, int desired);
int  IR_MATCH_MARK  (int measured_ticks, int desired_us);
int  IR_MATCH_SPACE (int measured_ticks, int desired_us);

//------------------------------------------------------------------------------
// Results returned from the decoder
//
typedef struct
{
	ir_decode_type_t       decode_type;  // UNKNOWN, NEC, SONY, RC5, ...
	unsigned int           address;      // Used by Panasonic & Sharp [16-bits]
	unsigned long          value;        // Decoded value [max 32-bits]
	int                    bits;         // Number of bits in decoded value
	volatile unsigned int  *rawbuf;      // Raw intervals in 500uS ticks
	int                    rawlen;       // Number of records in rawbuf
	int                    overflow;     // true iff IR raw code too long
} ir_decode_results;

extern volatile ir_decode_results irresults;

//------------------------------------------------------------------------------
// Functions for receiving IR
//
extern void IRrecv_IRrecvInit (GPIO_TypeDef* recvpinport, uint16_t recvpin);
extern void IRrecv_IRrecvInitBlink (GPIO_TypeDef* recvpinport, uint16_t recvpin, GPIO_TypeDef* blinkpinport, uint16_t blinkpin);

extern int      IRrecv_decode     (ir_decode_results *results);
extern void     IRrecv_enableIRIn (void);
extern uint8_t  IRrecv_isIdle     (void);
extern void     IRrecv_resume     (void);

long  IRrecv_decodeHash (ir_decode_results *results) ;
int   IRrecv_compare    (unsigned int oldval, unsigned int newval);


//......................................................................
#if IR_DECODE_SONY
	extern uint8_t IRrecv_decodeSony (ir_decode_results *results);
#endif

extern void IR_ISR (void);
extern void IRrecv_DataReadyCallback(unsigned long data);


//------------------------------------------------------------------------------
// Functions for sending IR
//
extern void IRsend_enableIROut (uint32_t khz);
extern void IRsend_mark        (unsigned int usec);
extern void IRsend_space       (unsigned int usec);
extern void IRsend_sendRaw     (const unsigned int buf[], unsigned int len, unsigned int hz);


//......................................................................
#if IR_SEND_SONY
extern void IRsend_sendSony (unsigned long data, int nbits);
#endif
//......................................................................

extern void Error_Handler(void);

#endif
