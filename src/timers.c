
#include "stm32f37x.h"

#include "timers.h"

void Init_Timers()
{
	// Set up timers

	// Timer 4, 1khz
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
	// Timer 4 runs at 72 mhz, so 1khz = 72000 divider (too big)
	// so we do a /16 prescaler, then a 4500 ARR
	TIM4->CR1 = 0;
	TIM4->CR2 = 0;
	TIM4->SMCR = 0;
	TIM4->DIER = TIM_DIER_UIE;	// Enable update interrupt
	TIM4->CCMR1 = 0;
	TIM4->CCMR2 = (6 << 12);	// Set ch4 to PWM mode (generate edges at tim. freq)
	TIM4->PSC = 15;		// this is /16, since it's /(x + 1)
	TIM4->ARR = 4499;	// period is 4500
	TIM4->CCR4 = 1000;	// Put an edge somewhere (this value doesn't matter so long as >0 and < ARR)


	// Timer 19, 10khz
	RCC->APB2ENR |= RCC_APB2ENR_TIM19EN;
	// Timer 19 runs at 72 mhz, so 10khz = 7200 divider
	// We do a /16 prescaler, then 450 ARR
	TIM19->CR1 = 0;
	TIM19->CR2 = 0;
	TIM19->SMCR = 0;
	TIM19->DIER = 0;
	TIM19->CCMR1 = (6 << 4);	// ch1 = pmw mode 1
	TIM19->CCMR2 = 0;
	TIM19->PSC = 15;
	TIM19->ARR = 449;	// period is 450
	TIM19->CCR1 = 100;


	// Enable tim4 interrupts
	NVIC_EnableIRQ(TIM4_IRQn);
}

void Start_Timers()
{
	// Start timers
	TIM19->CR1 |= TIM_CR1_CEN;
	TIM4->CR1 |= TIM_CR1_CEN;
}

void TIM4_IRQHandler()
{
	if(TIM4->SR | TIM_SR_UIF)
	{
		// Clear flag
		TIM4->SR &= ~TIM_SR_UIF;

		Events_1khz();
	}
}
