/*
 * adc.c
 *
 *  Created on: Dec 21, 2016
 *      Author: matth
 */

#include "stm32f37x.h"

#include "adc.h"

// Stores the samples from channels 1, 2, 3, 4, 5, 6, 8, 9, temp (16) (1khz)
volatile uint16_t regular_channels_sample[9];

volatile uint16_t map_average;

static inline void Init_ADC_GPIO()
{
	// Enable GPIOs
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
	GPIOA->MODER |= 0x00003FFF;		// Set PA0-6 to analog in
	GPIOB->MODER |= 0x0000000F;		// Set PB0-1 to analog in
}

static inline void Init_ADC_ADC()
{
	// Power ADC
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;


	// Reset status
	ADC1->SR = 0;
	// Enable interrupt for injected/reg channels, enable scan mode (hit all channels in seq for one trigger)
	ADC1->CR1 = ADC_CR1_JEOCIE | ADC_CR1_SCAN;
	// Enable ext triggers, regular = tim19trgo, injected = tim4trgo
	ADC1->CR2 = ADC_CR2_EXTTRIG | ADC_CR2_JEXTTRIG | (0 << 17) | (5 << 12)
			| ADC_CR2_DMA;	// Enable DMA



	// Turn on ADC and temp sensor
	ADC1->CR2 |= ADC_CR2_ADON | ADC_CR2_TSVREFE;

	// Reset cal
	ADC1->CR2 |= ADC_CR2_RSTCAL;
	// Wait for cal to reset
	while(ADC1->CR2 & ADC_CR2_RSTCAL);

	// Calibrate ADC
	ADC1->CR2 |= ADC_CR2_CAL;
	// Wait for cal to complete
	while(ADC1->CR2 & ADC_CR2_CAL);


	// Sample time = 13.5 cycle, except for temp sensor which is 239.5
	ADC1->SMPR1 = (7 << 18);
	ADC1->SMPR2 = (3 << 27) | (3 << 24) | (3 << 21) | (3 << 18) | (3 << 15)
			| (3 << 12) | (3 << 9) | (3 << 6) | (3 << 3) | (3 << 0);

	// Set up regular sequence
	ADC1->SQR1 = (8 << 20);				// 9 channels in reg. seq.
	ADC1->SQR2 = (16 << 10) | (9 << 5) | (8 << 0);	// 9th: temp sensor (ch16), 8th: ch9, 7th: ch8
	ADC1->SQR3 = (6 << 25) | (5 << 20) | (4 << 15) | (3 << 10) | (2 << 5)
			| (1 << 0); 	// Ch 6, 5, 4, 3, 2, 1

	// Set up injected sequence
	ADC1->JSQR = 0;		// Only convert 1 channel, ch0
	ADC1->JOFR1 = 0;	// Zero offset on injected channel 0
}

static inline void Init_ADC_DMA()
{
	// Set up DMA for regular channels
	// Turn on DMA
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	DMA1_Channel1->CCR = DMA_CCR_PL_1 | DMA_CCR_PL_0 | // Very high priority
	DMA_CCR_MSIZE_0 |	// 16 bit mem write size
			DMA_CCR_PSIZE_0 |	// 16 bit periph read size
			DMA_CCR_MINC |		// Memory increment mode
			DMA_CCR_CIRC;		// Circular

	// 9 transfers, 7 channels + battery voltage + internal temp
	DMA1_Channel1->CNDTR = 9;
	// DMA src = ADC 1 data reg
	DMA1_Channel1->CPAR = (uint32_t)(&ADC1->DR);
	// Write to the sample array
	DMA1_Channel1->CMAR = (uint32_t)regular_channels_sample;

	// Enable DMA channel
	DMA1_Channel1->CCR |= DMA_CCR_EN;
}

void Init_ADC_TempSensor();

void Init_ADC()
{
	/* Here's how this all works:

	 General purpose inputs are on ADC1 ch 1, 2, 3, 4, 5, 6, and 8.
	 Battery voltage is on ADC1 ch 9.
	 Temp sensor is on ADC1 ch 16.

	 These are sampled at 1khz


	 The MAP sensor is on ADC1 ch 0.

	 This is timer triggered to sample at 10khz.  An interrupt fires
	 for every conversion, and it's added to a sum and count of conversions.
	 Every ignition event, we update the MAP from this data, and reset.
	 If we aren't synced, we do this step instead at 1khz    */

	Init_ADC_GPIO();

	Init_ADC_ADC();

	Init_ADC_TempSensor();

	Init_ADC_DMA();

	// Enable ADC interrupt
	NVIC_EnableIRQ(ADC1_IRQn);
}

static uint32_t map_accumulator = 0;
static uint32_t map_sample_count = 0;

void ADC_UpdateMapAverage()
{
	map_average = map_accumulator / map_sample_count;

	map_accumulator = 0;
	map_sample_count = 0;
}

static float v30 = 0;
static float dTempdv = 0;

float ADC_CountsToVolts(uint16_t counts)
{
	return counts / 1240.909f; // = counts * 3.3 / 4095
}

void Init_ADC_TempSensor()
{
	// Read factory temp measurements
	// Taken at 30 and 110, plus or minus 5 deg c
	uint16_t val_30 = *((uint16_t*)0x1FFFF7B8);
	uint16_t val_110 = *((uint16_t*)0x1FFFF7C2);

	v30 = ADC_CountsToVolts(val_30);
	float v110 = ADC_CountsToVolts(val_110);

	dTempdv = 80 / (v110 - v30);
}

#include "status.h"

static int hasTempAvg = 0;

void ADC_UpdateTempSensor()
{
	uint16_t temp_count = regular_channels_sample[8];
	float volts = temp_count / 1240.909f;

	float dv = volts - v30;
	float delta = dv * dTempdv;

    float temperatureSample = delta + 30;

    if(hasTempAvg)
    {
    	status.system.cpu_temp = 0.001f * temperatureSample + 0.999f * status.system.cpu_temp;
    }
    else
    {
    	status.system.cpu_temp = temperatureSample;
    	hasTempAvg = 1;
    }
}

#include "status.h"

void ADC1_IRQHandler()
{
	CPU_busy_int();

	// Accumulate injected group (MAP sensor)
	if (ADC1->SR | ADC_SR_JEOC)
	{
		ADC1->SR &= ~ADC_SR_JEOC;	// Clear flag
		map_accumulator += ADC1->JDR1;
		map_sample_count++;
	}

	CPU_idle_int();
}

