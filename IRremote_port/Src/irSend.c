#include "IRremote.h"
#include "IRremoteInt.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"

// Code based on https://github.com/z3t0/Arduino-IRremote !

//+=============================================================================
void IRsend_sendRaw (const unsigned int buf[], unsigned int len, unsigned int khz)
{
	// Set IR carrier frequency
	IRsend_enableIROut(khz);

	for (unsigned int i = 0;  i < len;  i++)
	{
		if (i & 1)  IRsend_space(buf[i]) ;
		else        IRsend_mark (buf[i]) ;
	}

	IRsend_space(0);  // Always end with the LED off
}


//+=============================================================================
// Sends an IR mark for the specified number of microseconds.
// The mark output is modulated at the PWM frequency.
//
void IRsend_mark (unsigned int time)
{
	HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_2); // Enable PWM output
	if (time > 0) HAL_Delay(time/10);
}

//+=============================================================================
// Leave pin off for time (given in microseconds)
// Sends an IR space for the specified number of microseconds.
// A space is no output, so the PWM output is disabled.
//
void IRsend_space (unsigned int time)
{
	HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_2); // Disable PWM output
	if (time > 0) HAL_Delay(time/10);
}

//+=============================================================================
// Enables IR output.  The khz value controls the modulation frequency in kilohertz.
// To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
//
void IRsend_enableIROut (uint32_t khz)
{
	//TODO!!! ?
	// Disable the TIM2 Interrupt (which is used for receiving IR)
	//HAL_NVIC_DisableIRQ(TIM2_IRQn);

	//------------------------------------------------------------------
	// TIM3 initialization
	//

	GPIO_InitTypeDef GPIO_IR_TIMER_PWM;
	TIM_OC_InitTypeDef IR_TIMER_PWM_CH;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();

	GPIO_IR_TIMER_PWM.Pin = GPIO_PIN_7;
	GPIO_IR_TIMER_PWM.Mode = GPIO_MODE_AF_PP;
	GPIO_IR_TIMER_PWM.Pull = GPIO_NOPULL;
	GPIO_IR_TIMER_PWM.Speed = GPIO_SPEED_HIGH;
	GPIO_IR_TIMER_PWM.Alternate = GPIO_AF2_TIM3;

	HAL_GPIO_Init(GPIOA, &GPIO_IR_TIMER_PWM);

	HAL_TIM_OC_DeInit(&htim3);

	/* PWM_frequency = timer_tick_frequency / (TIM_Period + 1) */

	htim3.Instance = TIM3;
	uint32_t period = 1000 / khz;
	htim3.Init.Period = period & 0xFFFF;
	htim3.Init.Prescaler = 100;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

	HAL_TIM_Base_Init(&htim3);
	HAL_TIM_OC_Init(&htim3);

	/* PWM mode 2 = Clear on compare match */
	/* PWM mode 1 = Set on compare match */
	IR_TIMER_PWM_CH.OCMode = TIM_OCMODE_PWM1;

	/* To get proper duty cycle, you have simple equation */
	/* pulse_length = ((TIM_Period + 1) * DutyCycle) / 100 - 1 */
	/* where DutyCycle is in percent, between 0 and 100% */

	IR_TIMER_PWM_CH.Pulse = (((uint32_t)period)/2) & 0xFFFF;
	IR_TIMER_PWM_CH.OCPolarity = TIM_OCPOLARITY_HIGH;
	IR_TIMER_PWM_CH.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	IR_TIMER_PWM_CH.OCFastMode = TIM_OCFAST_DISABLE;
	IR_TIMER_PWM_CH.OCIdleState = TIM_OCIDLESTATE_RESET;
	IR_TIMER_PWM_CH.OCNIdleState = TIM_OCNIDLESTATE_RESET;

	HAL_TIM_OC_ConfigChannel(&htim3, &IR_TIMER_PWM_CH, TIM_CHANNEL_2);
	TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_2, TIM_CCxN_ENABLE | TIM_CCx_ENABLE );

	HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_2); // start generating IR carrier

	//------------------------------------------------------------------
}
