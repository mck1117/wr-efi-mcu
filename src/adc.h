

#ifndef _ADC_H
#define _ADC_H

void Init_ADC();

void ADC_UpdateMapAverage();

volatile uint16_t regular_channels_sample[8];
volatile uint16_t map_average;


#endif
