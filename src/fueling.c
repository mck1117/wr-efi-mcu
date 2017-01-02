
#include "status.h"

#include "tune.h"

static float Compute_AE_MAP()
{
	return 0;
}

static float Compute_AE_TPS()
{
	return 0;
}

void Compute_Fueling()
{
	// fuel = base_fuel * (ve * iat * clt * map + ae_map + ae_tps);

	// Compute AE first
	status.computations.ae_tps = Compute_AE_TPS();
	status.computations.ae_map = Compute_AE_MAP();


	// If the engine is cranking, most sensor inputs are bogus, so give cranking pulse width
	if(status.flags.cranking)
	{
		status.computations.fuel_qty_actual = lut_table1d16(&tune.cranking, status.input.clt);
		status.computations.fuel_factor = 0;
	}
	else
	{
		// Otherwise do normal fuel calc
		status.computations.ve = lut_table2d16_uint8(&tune.fuel, status.input.rpm, status.input.map) / 100;
		float iat = lut_table1d16(&tune.iat, status.input.iat) / 100;
		float clt = lut_table1d16(&tune.clt, status.input.clt) / 100;

		// Do AFR multiplier from AFR table
		float afr_target_x10 = lut_table2d16_uint8(&tune.afr_target, status.input.rpm, status.input.map);
		status.computations.afr_target = afr_target_x10 / 10;
		// We divide by the 10x afr target because stoich is also 10x
		status.computations.lambda_correction = tune.afr_stoich / afr_target_x10;

		status.computations.gamma = iat * clt;

		status.computations.fuel_factor = status.computations.ve * status.computations.lambda_correction * status.computations.gamma * status.input.map;

		status.computations.fuel_qty_actual = status.computations.fuel_factor;
	}

	// Add in AE values
	status.computations.fuel_qty_actual += (status.computations.ae_map + status.computations.ae_tps);
	// Multiply by base fuel (it's a ratio to base fuel right now)
	float fuel_qty_us = status.computations.fuel_qty_actual * tune.engine.base_fuel;
	status.computations.fuel_qty_actual = fuel_qty_us / 1000000;

	// Correct injector deadtime (convert to seconds in the process)
	status.output.injector_pw = (fuel_qty_us + lut_table1d16(&tune.injector_deadtime, status.input.batt)) / 1000000;

	// Only compute duty cycle if we're synced, as duty is meaningless if engine isn't spinning
	if(status.flags.synced)
	{
		// Period is doubled because inj fires every other turn
		status.computations.injector_duty = status.output.injector_pw / (2 * status.computations.period);
	}
	else
	{
		status.computations.injector_duty = 0;
	}
}
