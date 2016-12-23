
#include "stm32f37x.h"
#include "tune.h"
#include "adc.h"

#include "status.h"

static float sensor_values[8];

static float convert_sensor(uint16_t adc, tune_sensor_conversion_t* converter)
{
	// Full scale 5v input is 0.7575 * 4096 = 3012.2727
	// conversion factor = 5 / 3012.2727
	float voltage = adc * 0.00161172161172f;

	return lut_table1d16(converter, voltage);
}

void Sensors_Update()
{
	// Update raw sensor smoothing and conversion
	for(int i = 0; i < 8; i++)
	{
		uint16_t adc = regular_channels_sample[i];

		float raw_value = convert_sensor(adc, &tune.sconfig.sensor_conversion[i]);

		// Apply smoothing
		float s = tune.sconfig.smoothing_factors[i];
		sensor_values[i] = sensor_values[i] * (1 - s) + raw_value * s;
	}

	// Convert MAP sensor (no smoothing, this is done by the event sampling)
	status.input.map = convert_sensor(map_average, &tune.sconfig.sensor_map_conversion);

	// Look up regular sensors from config
	status.input.clt = sensor_values[tune.sconfig.idx_clt];
	status.input.iat = sensor_values[tune.sconfig.idx_iat];
	status.input.tps = sensor_values[tune.sconfig.idx_tps];
	status.input.afr1 = sensor_values[tune.sconfig.idx_afr1];
	status.input.afr2 = sensor_values[tune.sconfig.idx_afr2];
	status.input.batt = sensor_values[tune.sconfig.idx_batt];

	status.flags.cranking = status.input.rpm >= tune.engine.cranking_threshold;
	status.computations.period = 60 / status.input.rpm;
}


