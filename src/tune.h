
#ifndef __TUNE_H
#define __TUNE_H

#include <stdint.h>

typedef struct tune_table2d16_uint8_s {
	uint8_t values[16*16];
	int16_t x_axis[16];
	int16_t y_axis[16];
} tune_table2d16_uint8_t;

typedef struct tune_table2d16_int16_s {
	uint8_t values[16*16];
	int16_t x_axis[16];
	int16_t y_axis[16];
} tune_table2d16_int16_t;

typedef struct tune_table1d16_s {
	float values[16];
	int16_t x_axis[16];
} tune_table1d16_t;

/*typedef struct tune_table1d8_s {
	float values[16];
	float x_axis[16];
} tune_table1d8_t;*/


typedef tune_table1d16_t tune_sensor_conversion_t;


typedef struct tune_sensor_config_s {
	// Indecies for where each sensor is plugged in
	// clt, iat, tps, afr1, afr2, battery (in that order)
	uint8_t sensor_index[6];

	float smoothing_factors[6];

	tune_sensor_conversion_t sensor_map_conversion;
	tune_sensor_conversion_t sensor_conversion[6];
} tune_sensor_config_t;

typedef struct tune_engine_s {
	// Fuel pulse for no corrections, 100% VE
	float base_fuel;
	// Speed above which we switch from crank -> run
	float cranking_threshold;



	// Trigger settings
	// Number of teeth on the wheel
	uint16_t tooth_count;

	// Width of a tooth, in time quanta
	uint16_t tooth_width;

	// Number of teeth missing (long tooth width)
	uint16_t teeth_missing;

	// FPGA time quanta per revolution
	// 1 quantum should be small, << 1/10 degree
	uint16_t quanta_per_rev;

	// Relative cylinder phasing
	// Ex: inline/flat 6, wasted spark:
	// phase A = 0   deg
	// phase B = 120 deg
	// phase C = 240 deg
	// phase D = 0   deg (unused)
	uint16_t cyl_phase[4];

} tune_engine_t;

typedef struct tune_s {
	// Version goes at the beginning, never moves
	// so that ((uint32_t)tune) always gives the ver
	uint32_t version;

	// Cranking correction table
	tune_table1d16_t cranking;

	// Cranking ign timing
	float cranking_advance;
	// Cranking dwell
	float cranking_dwell;





	// Main fuel VE table
	tune_table2d16_uint8_t fuel;
	// AFR targets
	tune_table2d16_int16_t afr_target;
	float afr_stoich;


	// Batt volt vs. inj. deadtime table
	tune_table1d16_t injector_deadtime;

	// Clt, iat correction tables
	tune_table1d16_t clt, iat;



	// Main ignition timing table in tenths of a degree
	// table value of 156 = 15.6 BTDC
	tune_table2d16_int16_t ign;
	// Main dwell table
	tune_table1d16_t dwell;





	// Accel enrich
	//tune_table1d8_t ae_map, ae_tps;

	// Sensors configuration
	tune_sensor_config_t sconfig;
	// Engine configuration
	tune_engine_t engine;
} tune_t;

tune_t tune;

void Load_Tune();
void Save_Tune();

float lut_table2d16_int16(tune_table2d16_int16_t* table, float x, float y);
float lut_table2d16_uint8(tune_table2d16_uint8_t* table, float x, float y);
float lut_table1d16(tune_table1d16_t* table, float x);

#endif
