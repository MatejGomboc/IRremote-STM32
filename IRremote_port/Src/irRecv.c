#include "IRremote.h"
#include "IRremoteInt.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"

// Code based on https://github.com/z3t0/Arduino-IRremote !

// Allow all parts of the code access to the ISR data
// NB. The data can be changed by the ISR at any time, even mid-function
// Therefore we declare it as "volatile" to stop the compiler/CPU caching it
volatile irparams_t irparams;
volatile ir_decode_results irresults;

//+=============================================================================
// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in results
//
int IRrecv_decode (ir_decode_results* results)
{
	results->rawbuf   = irparams.rawbuf;
	results->rawlen   = irparams.rawlen;
	results->overflow = irparams.overflow;

	if (irparams.rcvstate != IR_STATE_STOP)  return 0 ;

#if IR_DECODE_SONY
	IR_DBG_PRINTLN("Attempting Sony decode")
	if (IRrecv_decodeSony(results))  return 1 ;
#endif

	//TODO ???
	// decodeHash returns a hash on any input.
	// Thus, it needs to be last in the list.
	// If you add any decodes, add them before this.
	//if (IRrecv_decodeHash(results))  return 1 ;

	// Throw away and start over
	IRrecv_resume();
	return 0;
}

//+=============================================================================
void IRrecv_IRrecvInit (GPIO_TypeDef* recvpinport, uint16_t recvpin)
{
	irparams.recvpinport = recvpinport;
	irparams.recvpin = recvpin;
	irparams.blinkflag = 0;
}

void IRrecv_IRrecvInitBlink (GPIO_TypeDef* recvpinport, uint16_t recvpin, GPIO_TypeDef* blinkpinport, uint16_t blinkpin)
{
	irparams.recvpinport = recvpinport;
	irparams.recvpin = recvpin;
	irparams.blinkpinport = blinkpinport;
	irparams.blinkpin = blinkpin;

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = blinkpin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(blinkpinport, &GPIO_InitStruct);

	irparams.blinkflag = 0;
}



//+=============================================================================
// initialization
//
void  IRrecv_enableIRIn()
{
	// Initialize state machine variables
	irparams.rcvstate = IR_STATE_IDLE;
	irparams.rawlen = 0;

	// Set pin modes
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = irparams.recvpin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(irparams.recvpinport, &GPIO_InitStruct);

	// Setup pulse clock timer interrupt
	// Prescale /500 (100M/500 = 5 microseconds per tick)
	// Period = 50us

	TIM_ClockConfigTypeDef sClockSourceConfig;

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 500;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 10;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

//+=============================================================================
// Return if receiving new IR signals
//
uint8_t  IRrecv_isIdle ( )
{
	return (irparams.rcvstate == IR_STATE_IDLE || irparams.rcvstate == IR_STATE_STOP) ? 1 : 0;
}

//+=============================================================================
// Restart the ISR state machine
//
void  IRrecv_resume ( )
{
	irparams.rcvstate = IR_STATE_IDLE;
	irparams.rawlen = 0;
}

//+=============================================================================
// hashdecode - decode an arbitrary IR code.
// Instead of decoding using a standard encoding scheme
// (e.g. Sony, NEC, RC5), the code is hashed to a 32-bit value.
//
// The algorithm: look at the sequence of MARK signals, and see if each one
// is shorter (0), the same length (1), or longer (2) than the previous.
// Do the same with the SPACE signals.  Hash the resulting sequence of 0's,
// 1's, and 2's to a 32-bit value.  This will give a unique value for each
// different code (probably), for most code systems.
//
// http://arcfn.com/2010/01/using-arbitrary-remotes-with-arduino.html
//
// Compare two tick values, returning 0 if newval is shorter,
// 1 if newval is equal, and 2 if newval is longer
// Use a tolerance of 20%
//
//int  IRrecv_compare (unsigned int oldval,  unsigned int newval)
//{
//	if      (newval < oldval * .8)  return 0 ;
//	else if (oldval < newval * .8)  return 2 ;
//	else                            return 1 ;
//}

//+=============================================================================
// Use FNV hash algorithm: http://isthe.com/chongo/tech/comp/fnv/#FNV-param
// Converts the raw code values into a 32-bit hash code.
// Hopefully this code is unique for each button.
// This isn't a "real" decoding, just an arbitrary value.
//
//#define IR_FNV_PRIME_32 16777619
//#define IR_FNV_BASIS_32 2166136261
//
//long  IRrecv_decodeHash (ir_decode_results *results)
//{
//	long hash = IR_FNV_BASIS_32;
//
//	// Require at least 6 samples to prevent triggering on noise
//	if (results->rawlen < 6)  return 0 ;
//
//	for (int i = 1;  (i + 2) < results->rawlen;  i++) {
//		int value =  IRrecv_compare(results->rawbuf[i], results->rawbuf[i+2]);
//		// Add value into the hash
//		hash = (hash * IR_FNV_PRIME_32) ^ value;
//	}
//
//	results->value       = hash;
//	results->bits        = 32;
//	results->decode_type = UNKNOWN;
//
//	return 1;
//}
