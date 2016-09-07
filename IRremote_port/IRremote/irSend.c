#include "IRremote.h"
#include "IRremoteInt.h"
#include "stm32f4xx_hal_tim.h"

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
void  IRsend_mark (unsigned int time)
{
	TIM_HandleTypeDef htim4;
	HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_1); // Enable PWM output
	if (time > 0) IRsend_custom_delay_usec(time);
}

//+=============================================================================
// Leave pin off for time (given in microseconds)
// Sends an IR space for the specified number of microseconds.
// A space is no output, so the PWM output is disabled.
//
void  IRsend_space (unsigned int time)
{
	TIM_HandleTypeDef htim4;
	HAL_TIM_OC_Stop(&htim4, TIM_CHANNEL_1); // Disable pin 3 PWM output
	if (time > 0) IRsend_custom_delay_usec(time);
}

//+=============================================================================
// Enables IR output.  The khz value controls the modulation frequency in kilohertz.
// The IR output will be on pin 3 (OC2B).
// This routine is designed for 36-40KHz; if you use it for other values, it's up to you
// to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
// TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
// controlling the duty cycle.
// There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
// To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
//
void  IRsend_enableIROut (int khz)
{
	// Disable the TIM2 Interrupt (which is used for receiving IR)
	HAL_NVIC_DisableIRQ(TIM2_IRQn);

	TIM_HandleTypeDef htim4;
	GPIO_InitTypeDef GPIO_IR_TIMER_PWM;
	TIM_OC_InitTypeDef IR_TIMER_PWM_CH;

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();

	GPIO_IR_TIMER_PWM.Pin = GPIO_PIN_6;
	GPIO_IR_TIMER_PWM.Mode = GPIO_MODE_AF_PP;
	GPIO_IR_TIMER_PWM.Pull = GPIO_NOPULL;
	GPIO_IR_TIMER_PWM.Speed = GPIO_SPEED_HIGH;
	GPIO_IR_TIMER_PWM.Alternate = GPIO_AF2_TIM4;

	HAL_GPIO_Init(GPIOB, &GPIO_IR_TIMER_PWM);

	HAL_TIM_OC_DeInit(&htim4);

	/* PWM_frequency = timer_tick_frequency / (TIM_Period + 1) */

	htim4.Instance = TIM4;
	htim4.Init.Period = ((uint32_t)2333);
	htim4.Init.Prescaler = 0;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

	HAL_TIM_Base_Init(&htim4);
	HAL_TIM_OC_Init(&htim4);

	/* PWM mode 2 = Clear on compare match */
	/* PWM mode 1 = Set on compare match */
	IR_TIMER_PWM_CH.OCMode = TIM_OCMODE_PWM1;

	/* To get proper duty cycle, you have simple equation */
	/* pulse_length = ((TIM_Period + 1) * DutyCycle) / 100 - 1 */
	/* where DutyCycle is in percent, between 0 and 100% */

	IR_TIMER_PWM_CH.Pulse = ((uint32_t)2333)/4;
	IR_TIMER_PWM_CH.OCPolarity = TIM_OCPOLARITY_HIGH;
	IR_TIMER_PWM_CH.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	IR_TIMER_PWM_CH.OCFastMode = TIM_OCFAST_DISABLE;
	IR_TIMER_PWM_CH.OCIdleState = TIM_OCIDLESTATE_RESET;
	IR_TIMER_PWM_CH.OCNIdleState = TIM_OCNIDLESTATE_RESET;

	HAL_TIM_OC_ConfigChannel(&htim4, &IR_TIMER_PWM_CH, TIM_CHANNEL_1);
	TIM_SET_CAPTUREPOLARITY(&htim4, TIM_CHANNEL_1, TIM_CCxN_ENABLE | TIM_CCx_ENABLE );

	HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_1); // start generating IR carrier
}

//+=============================================================================
// Custom delay function that circumvents Arduino's delayMicroseconds limit

///TODO !!!

void IRsend_custom_delay_usec(uint32_t uSecs)
{
  if (uSecs > 4)
  {
	uint32_t start = HAL_GetTick() * 1000U;
	uint32_t endMicros = start + uSecs - 4;

    if (endMicros < start)
    { // Check if overflow
      while (HAL_GetTick() * 1000U > start) {} // wait until overflow
    }

    while (HAL_GetTick() * 1000U < endMicros) {} // normal wait
  }
  //else {
  //  __asm__("nop\n\t"); // must have or compiler optimizes out
  //}
}

