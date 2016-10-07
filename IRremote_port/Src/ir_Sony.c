#include "IRremote.h"
#include "IRremoteInt.h"

// Code based on https://github.com/z3t0/Arduino-IRremote !
//
//==============================================================================
//                           SSSS   OOO   N   N  Y   Y
//                          S      O   O  NN  N   Y Y
//                           SSS   O   O  N N N    Y
//                              S  O   O  N  NN    Y
//                          SSSS    OOO   N   N    Y
//==============================================================================

#define IR_SONY_BITS                   12
#define IR_SONY_HDR_MARK             2400
#define IR_SONY_HDR_SPACE             600
#define IR_SONY_ONE_MARK             1200
#define IR_SONY_ZERO_MARK             600
#define IR_SONY_RPT_LENGTH          45000
#define IR_SONY_DOUBLE_SPACE_USECS    500  // usually ssee 713 - not using ticks as get number wrapround

//+=============================================================================
#if IR_SEND_SONY
void  IRsend_sendSony (unsigned long data,  int nbits)
{
	// Set IR carrier frequency
	IRsend_enableIROut(40);

	// Header
	IRsend_mark(IR_SONY_HDR_MARK);
	IRsend_space(IR_SONY_HDR_SPACE);

	// Data
	for (unsigned long  mask = 1UL << (nbits - 1);  mask;  mask >>= 1)
	{
		if (data & mask)
		{
			IRsend_mark(IR_SONY_ONE_MARK);
			IRsend_space(IR_SONY_HDR_SPACE);
		}
		else
		{
			IRsend_mark(IR_SONY_ZERO_MARK);
			IRsend_space(IR_SONY_HDR_SPACE);
    	}
  	}

	// We will have ended with LED off
}
#endif

//+=============================================================================
#if IR_DECODE_SONY
uint8_t  IRrecv_decodeSony (ir_decode_results *results)
{
	long  data   = 0;
	int   offset = 0;  // Dont skip first space, check its size

	if (irparams.rawlen < (2 * IR_SONY_BITS) + 2)  return 0 ;

	// Some Sony's deliver repeats fast after first
	// unfortunately can't spot difference from of repeat from two fast clicks
	if (results->rawbuf[offset] < IR_SONY_DOUBLE_SPACE_USECS) return 0;

	offset++;

	// Initial mark
	if (!IR_MATCH_MARK(results->rawbuf[offset++], IR_SONY_HDR_MARK))  return 0 ;

	while (offset + 1 < irparams.rawlen)
	{
		if (!IR_MATCH_SPACE(results->rawbuf[offset++], IR_SONY_HDR_SPACE))  break ;

		if (IR_MATCH_MARK(results->rawbuf[offset], IR_SONY_ONE_MARK)) data = (data << 1) | 1 ;
		else if (IR_MATCH_MARK(results->rawbuf[offset], IR_SONY_ZERO_MARK)) data = (data << 1) | 0 ;
		else return 0 ;

		offset++;
	}

	// Success
	results->bits = (offset - 1) / 2;
	if (results->bits < 12)
	{
		results->bits = 0;
		return 0;
	}
	results->value       = data;
	results->decode_type = SONY;
	return 1;
}

#endif
