/*
 * status.c
 *
 *  Created on: Dec 30, 2016
 *      Author: matth
 */

#include "status.h"

#include "stm32f37x.h"

void Init_CPUUsage()
{
	// Timer 2, 64 mhz, CPU usage timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// Nothing fancy, juts run at SYSCLK speed
	TIM2->CR1 = 0;
	TIM2->CR2 = 0;
	TIM2->SMCR = 0;
	TIM2->DIER = 0;
	TIM2->PSC = 0;
	TIM2->ARR = 0xFFFFFFFF;

	CPU_idle();
}

void CPU_idle()
{
	TIM2->CR1 |= TIM_CR1_CEN;
}

void CPU_busy()
{
	TIM2->CR1 &= ~TIM_CR1_CEN;
}

#include "stdio.h"
#include "serial.h"

void CPU_Usage_Update()
{
	static int i = 0;

	if(i == 20)
	{
		uint32_t cnt = TIM2->CNT;
		TIM2->CNT = 0;

		uint32_t oneSecond = 12800;

		uint8_t free = cnt / oneSecond;

		uint8_t usage = 100 - free;

		status.system.cpu_usage = usage;

		char buf[50];
		sprintf(buf, "%u %u\r\n", cnt, usage);
		Serial_SendStr(buf);

		i = 0;
	}

	i++;
}
