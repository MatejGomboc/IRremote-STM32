//******************************************************************************
// Code based on https://github.com/z3t0/Arduino-IRremote !
//
// IRremote
// Version 2.0.1 June, 2015
// Copyright 2009 Ken Shirriff
// For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
//
// Modified by Paul Stoffregen <paul@pjrc.com> to support other boards and timers
// Modified  by Mitra Ardron <mitra@mitra.biz>
// Added Sanyo and Mitsubishi controllers
// Modified Sony to spot the repeat codes that some Sony's send
//
// Interrupt code based on NECIRrcv by Joe Knapp
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
// Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
//
// JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
// LG added by Darryl Smith (based on the JVC protocol)
// Whynter A/C ARC-110WD added by Francesco Meschia
//******************************************************************************

#include "IRremote.h"
#include "IRremoteInt.h"
#include "stm32f4xx_hal_tim.h"

//+=============================================================================
// The match functions were (apparently) originally MACROs to improve code speed
//   (although this would have bloated the code) hence the names being CAPS
// A later release implemented debug output and so they needed to be converted
//   to functions.
// I tried to implement a dual-compile mode (DEBUG/non-DEBUG) but for some
//   reason, no matter what I did I could not get them to function as macros again.
// I have found a *lot* of bugs in the Arduino compiler over the last few weeks,
//   and I am currently assuming that one of these bugs is my problem.
// I may revisit this code at a later date and look at the assembler produced
//   in a hope of finding out what is going on, but for now they will remain as
//   functions even in non-DEBUG mode
//
int IR_MATCH (int measured,  int desired)
{
	IR_DBG_PRINT("Testing: ");
	IR_DBG_PRINT_INT(IR_TICKS_LOW(desired));
	IR_DBG_PRINT(" <= ");
	IR_DBG_PRINT_INT(measured);
	IR_DBG_PRINT(" <= ");
	IR_DBG_PRINT_INT(IR_TICKS_HIGH(desired));

	uint8_t passed = ((measured >= IR_TICKS_LOW(desired)) && (measured <= IR_TICKS_HIGH(desired)));

	if (passed)
		IR_DBG_PRINTLN("?; passed");
	else
		IR_DBG_PRINTLN("?; FAILED");

	return passed;
}

//+========================================================
// Due to sensor lag, when received, Marks tend to be 100us too long
//
int IR_MATCH_MARK (int measured_ticks,  int desired_us)
{
	IR_DBG_PRINT("Testing mark (actual vs desired): ");
	IR_DBG_PRINT_INT(measured_ticks * IR_USECPERTICK);
	IR_DBG_PRINT("us vs ");
	IR_DBG_PRINT_INT(desired_us);
	IR_DBG_PRINT("us");
	IR_DBG_PRINT(": ");
	IR_DBG_PRINT_INT(IR_TICKS_LOW(desired_us + IR_MARK_EXCESS) * IR_USECPERTICK);
	IR_DBG_PRINT(" <= ");
	IR_DBG_PRINT_INT(measured_ticks * IR_USECPERTICK);
	IR_DBG_PRINT(" <= ");
	IR_DBG_PRINT_INT(IR_TICKS_HIGH(desired_us + IR_MARK_EXCESS) * IR_USECPERTICK);

	uint8_t passed = ((measured_ticks >= IR_TICKS_LOW(desired_us + IR_MARK_EXCESS))
			&& (measured_ticks <= IR_TICKS_HIGH(desired_us + IR_MARK_EXCESS)));

	if (passed)
		IR_DBG_PRINTLN("?; passed");
	else
		IR_DBG_PRINTLN("?; FAILED");

	return passed;
}

//+========================================================
// Due to sensor lag, when received, Spaces tend to be 100us too short
//
int IR_MATCH_SPACE (int measured_ticks,  int desired_us)
{
	IR_DBG_PRINT("Testing space (actual vs desired): ");
	IR_DBG_PRINT_INT(measured_ticks * IR_USECPERTICK);
	IR_DBG_PRINT("us vs ");
	IR_DBG_PRINT_INT(desired_us);
	IR_DBG_PRINT("us");
	IR_DBG_PRINT(": ");
	IR_DBG_PRINT_INT(IR_TICKS_LOW(desired_us - IR_MARK_EXCESS) * IR_USECPERTICK);
	IR_DBG_PRINT(" <= ");
	IR_DBG_PRINT_INT(measured_ticks * IR_USECPERTICK);
	IR_DBG_PRINT(" <= ");
	IR_DBG_PRINT_INT(IR_TICKS_HIGH(desired_us - IR_MARK_EXCESS) * IR_USECPERTICK);

	uint8_t passed = ((measured_ticks >= IR_TICKS_LOW(desired_us - IR_MARK_EXCESS))
			&& (measured_ticks <= IR_TICKS_HIGH(desired_us - IR_MARK_EXCESS)));

	if (passed)
		IR_DBG_PRINTLN("?; passed");
	else
		IR_DBG_PRINTLN("?; FAILED");

	return passed;
}

//+=============================================================================
// Interrupt Service Routine - Fires every 50uS
// TIM2 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 50uS [microseconds, 0.000050 seconds]
// 'rawlen' counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a the first [SPACE] entry gets long:
//   Ready is set; State switches to IDLE; Timing of SPACE continues.
// As soon as first MARK arrives:
//   Gap width is recorded; Ready is cleared; New logging starts
//

void IR_Recv_ISR ()
{
	// Read if IR Receiver -> SPACE [xmt LED off] or a MARK [xmt LED on]
	uint8_t irdata = (uint8_t)HAL_GPIO_ReadPin(irparams.recvpinport, irparams.recvpin);

	irparams.timer++; // One more 50uS tick
	if (irparams.rawlen >= IR_RAWBUF) irparams.rcvstate = IR_STATE_OVERFLOW ; // Buffer overflow

	switch(irparams.rcvstate)
	{
		//......................................................................
		case IR_STATE_IDLE: // In the middle of a gap
			if (irdata == IR_MARK)
			{
				if (irparams.timer < IR_GAP_TICKS)
				{  // Not big enough to be a gap.
					irparams.timer = 0;
				}
				else
				{
					// Gap just ended; Record duration; Start recording transmission
					irparams.overflow                  = 0;
					irparams.rawlen                    = 0;
					irparams.rawbuf[irparams.rawlen++] = irparams.timer;
					irparams.timer                     = 0;
					irparams.rcvstate                  = IR_STATE_MARK;
				}
			}
			break;
		//......................................................................
		case IR_STATE_MARK:  // Timing Mark
			if (irdata == IR_SPACE)
			{   // Mark ended; Record time
				irparams.rawbuf[irparams.rawlen++] = irparams.timer;
				irparams.timer                     = 0;
				irparams.rcvstate                  = IR_STATE_SPACE;
			}
			break;
		//......................................................................
		case IR_STATE_SPACE:  // Timing Space
			if (irdata == IR_MARK)
			{   // Space just ended; Record time
				irparams.rawbuf[irparams.rawlen++] = irparams.timer;
				irparams.timer                     = 0;
				irparams.rcvstate                  = IR_STATE_MARK;
			}
			else if (irparams.timer > IR_GAP_TICKS)
			{   // Space
				// A long Space, indicates gap between codes
				// Flag the current code as ready for processing
				// Switch to STOP
				// Don't reset timer; keep counting Space width
				irparams.rcvstate = IR_STATE_STOP;
			}
			break;
		//......................................................................
		case IR_STATE_STOP:  // Waiting; Measuring Gap
		 	if (irdata == IR_MARK)  irparams.timer = 0 ;  // Reset gap timer
		 	break;
		//......................................................................
		case IR_STATE_OVERFLOW:  // Flag up a read overflow; Stop the State Machine
			irparams.overflow = 1;
			irparams.rcvstate = IR_STATE_STOP;
		 	break;
	}

	// If requested, flash LED while receiving IR data
	if (irparams.blinkflag)
	{
		if (irdata == IR_MARK)
		{
			if (irparams.blinkpin) HAL_GPIO_WritePin(irparams.blinkpinport, irparams.blinkpin, GPIO_PIN_SET); // Turn user defined pin LED on
		}
		else if (irparams.blinkpin) HAL_GPIO_WritePin(irparams.blinkpinport, irparams.blinkpin, GPIO_PIN_RESET); // Turn user defined pin LED on
	}

	//DO DECODE !
	if (IRrecv_decode(&irresults))
	{
		IRrecv_DataReadyCallback(irresults.value);
	}
	//////////
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM2)
	{
		IR_Recv_ISR();
	}
}
