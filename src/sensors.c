#include "stm32f37x.h"
#include "tune.h"
#include "adc.h"

#include "status.h"

static float convert_sensor_voltage(uint16_t adc)
{
	// Full scale 5v input is 0.7575 * 4096 = 3012.2727
	// conversion factor = 5 / 3012.2727
	return adc * 0.00161172161172f;
}

static float convert_sensor(uint16_t adc, tune_sensor_conversion_t* converter)
{
	float voltage = convert_sensor_voltage(adc);

	return lut_table1d16(converter, voltage);
}


static void smooth_sensor(float* sensor, float new_value, float alpha)
{
	// read old value
	float old_value = *sensor;

	float result = new_value * alpha + (1 - alpha) * old_value;

	// Update new smoothed value
	*sensor = result;
}


void Sensors_Update()
{
	float unsmoothed[6];

	// Look up and convert sensors from raw channel array
	for(int i = 0; i < 6; i++)
	{
		uint16_t raw_adc = regular_channels_sample[tune.sconfig.sensor_index[i]];

		unsmoothed[i] = convert_sensor(raw_adc, &tune.sconfig.sensor_conversion[i]);
	}

	smooth_sensor(&status.input.clt, unsmoothed[0], tune.sconfig.smoothing_factors[0]);
	smooth_sensor(&status.input.iat, unsmoothed[1], tune.sconfig.smoothing_factors[1]);
	smooth_sensor(&status.input.tps, unsmoothed[2], tune.sconfig.smoothing_factors[2]);
	smooth_sensor(&status.input.afr1, unsmoothed[3], tune.sconfig.smoothing_factors[3]);
	smooth_sensor(&status.input.afr2, unsmoothed[4], tune.sconfig.smoothing_factors[4]);
	smooth_sensor(&status.input.batt, unsmoothed[5], tune.sconfig.smoothing_factors[5]);

	// Convert MAP sensor (no smoothing, this is done by the event sampling)
	status.input.map = convert_sensor(map_average, &tune.sconfig.sensor_map_conversion);


	// Cranking it the engine is turning, but slowly
	status.flags.cranking = status.input.rpm > 0.1f &&
							status.input.rpm < tune.engine.cranking_threshold;

	status.flags.running = status.input.rpm >= tune.engine.cranking_threshold;

	// 60 seconds / RPM = seconds
	status.computations.period = 60 / status.input.rpm;
}


