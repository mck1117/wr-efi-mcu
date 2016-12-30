
#include "stm32f37x.h"

#include "timers.h"

void Init_Timers()
{
	// Set up timers

	// Timer 4, 10khz
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
	// Timer 4 runs at 64 mhz, so 1khz = 6400 divider (too big)
	// so we do a /16 prescaler, then a 400 ARR
	TIM4->CR1 = 0;
	TIM4->CR2 = (2 << 4);	// MMS = 010 (TRGO = update generate (makes a pulse when reset))
	TIM4->SMCR = 0;
	TIM4->DIER = TIM_DIER_UIE;	// Enable update interrupt
	TIM4->PSC = 15;		// this is /16, since it's /(x + 1)
	TIM4->ARR = 399;	// period is 400


	// Timer 19, 1khz
	RCC->APB2ENR |= RCC_APB2ENR_TIM19EN;
	// Timer 19 runs at 64 mhz, so 1khz = 64000 divider
	// We do a /16 prescaler, then 4000 ARR
	TIM19->CR1 = 0;
	TIM19->CR2 = (2 << 4);	// MMS = 010 (TRGO = update generate (makes a pulse when reset))
	TIM19->SMCR = 0;
	TIM19->DIER = TIM_DIER_UIE;
	TIM19->PSC = 15;
	TIM19->ARR = 3999;	// period is 4000


	// Enable tim4 interrupts
	NVIC_EnableIRQ(TIM19_IRQn);
}

void Start_Timers()
{
	// Start timers
	TIM19->CR1 |= TIM_CR1_CEN;
	TIM4->CR1 |= TIM_CR1_CEN;
}

void TIM19_IRQHandler()
{
	TIM_busy();

	if(TIM19->SR | TIM_SR_UIF)
	{
		// Clear flag
		TIM19->SR &= ~TIM_SR_UIF;

		Events_1khz();
	}
}
