#include "IRremote.h"
#include "IRremoteInt.h"

//+=============================================================================
// Gets one undecoded level at a time from the raw buffer.
// The RC5/6 decoding is easier if the data is broken into time intervals.
// E.g. if the buffer has MARK for 2 time intervals and SPACE for 1,
// successive calls to getRClevel will return MARK, MARK, SPACE.
// offset and used are updated to keep track of the current position.
// t1 is the time interval for a single bit in microseconds.
// Returns -1 for error (measured time interval is not a multiple of t1).
//
#if (IR_DECODE_RC5 || IR_DECODE_RC6)
int  IRrecv_getRClevel (ir_decode_results *results,  int *offset,  int *used,  int t1)
{
	int width;
	int val;
	int correction;
	int avail;

	if (*offset >= results->rawlen) return IR_SPACE ;  // After end of recorded buffer, assume SPACE.

	width       =  results->rawbuf[*offset];
	val         =  ((*offset) % 2) ? IR_MARK : IR_SPACE;
	correction  =  (val == IR_MARK) ? IR_MARK_EXCESS : - IR_MARK_EXCESS;

	if      (IR_MATCH(width, (  t1) + correction)) avail = 1;
	else if (IR_MATCH(width, (2*t1) + correction)) avail = 2;
	else if (IR_MATCH(width, (3*t1) + correction)) avail = 3;
	else                                           return -1;

	(*used)++;
	if(*used >= avail)
	{
		*used = 0;
		(*offset)++;
	}

	IR_DBG_PRINTLN((val == IR_MARK) ? "MARK" : "SPACE");

	return val;
}
#endif

//==============================================================================
// RRRR    CCCC  55555
// R   R  C      5
// RRRR   C      5555
// R  R   C          5
// R   R   CCCC  5555
//
// NB: First bit must be a one (start bit)
//
#define IR_MIN_RC5_SAMPLES     11
#define IR_RC5_T1             889
#define IR_RC5_RPT_LENGTH   46000

//+=============================================================================
#if IR_SEND_RC5
void IRsend_sendRC5 (unsigned long data,  int nbits)
{
	// Set IR carrier frequency
	IRsend_enableIROut(36);

	// Start
	IRsend_mark(IR_RC5_T1);
	IRsend_space(IR_RC5_T1);
	IRsend_mark(IR_RC5_T1);

	// Data
	for (unsigned long  mask = 1UL << (nbits - 1);  mask;  mask >>= 1)
	{
		if (data & mask)
		{
			IRsend_space(IR_RC5_T1); // 1 is space, then mark
			IRsend_mark(IR_RC5_T1);
		} else
		{
			IRsend_mark(IR_RC5_T1);
			IRsend_space(IR_RC5_T1);
		}
	}

	IRsend_space(0);  // Always end with the LED off
}
#endif

//+=============================================================================
#if IR_DECODE_RC5
uint8_t  IRrecv_decodeRC5 (ir_decode_results *results)
{
	int   nbits;
	long  data   = 0;
	int   used   = 0;
	int   offset = 1;  // Skip gap space

	if (irparams.rawlen < IR_MIN_RC5_SAMPLES + 2)  return 0 ;

	// Get start bits
	if (IRrecv_getRClevel(results, &offset, &used, IR_RC5_T1) != IR_MARK)   return 0 ;
	if (IRrecv_getRClevel(results, &offset, &used, IR_RC5_T1) != IR_SPACE)  return 0 ;
	if (IRrecv_getRClevel(results, &offset, &used, IR_RC5_T1) != IR_MARK)   return 0 ;

	for (nbits = 0;  offset < irparams.rawlen;  nbits++)
	{
		int  levelA = IRrecv_getRClevel(results, &offset, &used, IR_RC5_T1);
		int  levelB = IRrecv_getRClevel(results, &offset, &used, IR_RC5_T1);

		if      ((levelA == IR_SPACE) && (levelB == IR_MARK ))  data = (data << 1) | 1 ;
		else if ((levelA == IR_MARK ) && (levelB == IR_SPACE))  data = (data << 1) | 0 ;
		else                                              return 0 ;
	}

	// Success
	results->bits        = nbits;
	results->value       = data;
	results->decode_type = RC5;
	return 1;
}
#endif

//+=============================================================================
// RRRR    CCCC   6666
// R   R  C      6
// RRRR   C      6666
// R  R   C      6   6
// R   R   CCCC   666
//
// NB : Caller needs to take care of flipping the toggle bit
//
#define IR_MIN_RC6_SAMPLES      1
#define IR_RC6_HDR_MARK      2666
#define IR_RC6_HDR_SPACE      889
#define IR_RC6_T1             444
#define IR_RC6_RPT_LENGTH   46000

#if IR_SEND_RC6
void IRsend_sendRC6 (unsigned long data, int nbits)
{
	// Set IR carrier frequency
	IRsend_enableIROut(36);

	// Header
	IRsend_mark(IR_RC6_HDR_MARK);
	IRsend_space(IR_RC6_HDR_SPACE);

	// Start bit
	IRsend_mark(IR_RC6_T1);
	IRsend_space(IR_RC6_T1);

	// Data
	for (unsigned long  i = 1, mask = 1UL << (nbits - 1);  mask;  i++, mask >>= 1)
	{
		// The fourth bit we send is a "double width trailer bit"
		int  t = (i == 4) ? (IR_RC6_T1 * 2) : (IR_RC6_T1) ;
		if (data & mask)
		{
			IRsend_mark(t);
			IRsend_space(t);
		} else
		{
			IRsend_space(t);
			IRsend_mark(t);
		}
	}

	IRsend_space(0);  // Always end with the LED off
}
#endif

//+=============================================================================
#if IR_DECODE_RC6
uint8_t  IRrecv_decodeRC6 (ir_decode_results *results)
{
	int   nbits;
	long  data   = 0;
	int   used   = 0;
	int   offset = 1;  // Skip first space

	if (results->rawlen < IR_MIN_RC6_SAMPLES)  return 0 ;

	// Initial mark
	if (!IR_MATCH_MARK(results->rawbuf[offset++],  IR_RC6_HDR_MARK))   return 0 ;
	if (!IR_MATCH_SPACE(results->rawbuf[offset++], IR_RC6_HDR_SPACE))  return 0 ;

	// Get start bit (1)
	if (IRrecv_getRClevel(results, &offset, &used, IR_RC6_T1) != IR_MARK)   return 0 ;
	if (IRrecv_getRClevel(results, &offset, &used, IR_RC6_T1) != IR_SPACE)  return 0 ;

	for (nbits = 0;  offset < results->rawlen;  nbits++) {
		int  levelA, levelB;  // Next two levels

		levelA = IRrecv_getRClevel(results, &offset, &used, IR_RC6_T1);
		if (nbits == 3) {
			// T bit is double wide; make sure second half matches
			if (levelA != IRrecv_getRClevel(results, &offset, &used, IR_RC6_T1)) return 0;
		}

		levelB = IRrecv_getRClevel(results, &offset, &used, IR_RC6_T1);
		if (nbits == 3) {
			// T bit is double wide; make sure second half matches
			if (levelB != IRrecv_getRClevel(results, &offset, &used, IR_RC6_T1)) return 0;
		}

		if      ((levelA == IR_MARK ) && (levelB == IR_SPACE))  data = (data << 1) | 1 ;  // inverted compared to RC5
		else if ((levelA == IR_SPACE) && (levelB == IR_MARK ))  data = (data << 1) | 0 ;  // ...
		else                                              return 0 ;            // Error
	}

	// Success
	results->bits        = nbits;
	results->value       = data;
	results->decode_type = RC6;
	return 1;
}
#endif
