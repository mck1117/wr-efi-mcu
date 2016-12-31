#ifndef __STATUS_H
#define __STATUS_H

#include <stdint.h>

typedef struct status_inputs_s {
	float map, rpm, clt, iat, tps, afr1, afr2, batt;
} status_inputs_t;

typedef struct status_computations_s {
	// Period, seconds, per revolution
	float period;


	// Total fuel correction, pre VE (just clt, iat, etc)
	float gamma;
	// VE lut from table
	float ve;

	// AFR target from table
	float afr_target;
	// Adjustment for AFR target
	float lambda_correction;

	// AE adjust percent for TPS-based AE
	float ae_tps;
	// AE adjust percent for MAP-based AE
	float ae_map;

	// Total adjust factor applied to base fuel
	float fuel_factor;

	// Desired injector pulse width (without correction)
	float fuel_qty_actual;
	// Injector duty cycle.  0.3 = 30% duty
	float injector_duty;
} status_computations_t;

typedef struct status_outputs_s {
	float injector_pw;
	float ign_timing;
	float ign_dwell;
} status_outputs_t;

typedef struct status_flags_s {
	uint8_t synced:1;
	uint8_t cranking:1;
	uint8_t running:1;
} status_flags_t;

typedef struct status_system_s {
	uint8_t cpu_usage;
} status_system_t;

typedef struct status_s {
	status_inputs_t input;
	status_outputs_t output;

	status_computations_t computations;

	status_flags_t flags;

	status_system_t system;
} status_t;

status_t status;

void Init_CPUUsage();

void CPU_idle();
void CPU_busy();

void CPU_Usage_Update();

#endif
