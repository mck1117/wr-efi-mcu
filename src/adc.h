

#ifndef _ADC_H
#define _ADC_H

void Init_ADC();

void ADC_UpdateMapAverage();
void ADC_UpdateTempSensor();

float ADC_CountsToVolts(uint16_t counts);

volatile uint16_t regular_channels_sample[9];
volatile uint16_t map_average;


#endif
