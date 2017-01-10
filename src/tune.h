
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
	uint16_t values[16];
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

	float battery_voltage_ratio;
} tune_sensor_config_t;

typedef struct tune_engine_s {
	// Fuel pulse for no corrections, 100% VE, in microseconds (8 ms = 8000 value)
	uint16_t base_fuel;
	// RPM above which we switch from crank -> run
	uint16_t cranking_threshold;

	// Trigger settings
	// Number of teeth on the wheel
	uint16_t tooth_count;

	// Number of teeth missing (long tooth width)
	uint16_t teeth_missing;

	// Cylinder count, individual cylinder phases are computed later
	uint8_t cylinder_count;

	// Distance between missing tooth and the #1 spark output, in tenths of a degree
	// ie if the missing tooth happens 65.3 degrees before #1 TDC, then set this to 653
	int16_t trigger_offset;
} tune_engine_t;

uint16_t Tune_QuantaPerRev();

typedef struct tune_s {
	// Version goes at the beginning, never moves
	// so that ((uint32_t)tune) always gives the ver
	uint32_t version;

	// Cranking correction table
	tune_table1d16_t cranking;

	// Cranking ign timing, 10ths of a deg BTDC (13.5 deg btdc = 135 value)
	int16_t cranking_advance;
	// Cranking dwell, microseconds
	uint16_t cranking_dwell;





	// Main fuel VE table
	tune_table2d16_uint8_t fuel;
	// AFR targets (x10)
	tune_table2d16_uint8_t afr_target;
	uint8_t afr_stoich;


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
