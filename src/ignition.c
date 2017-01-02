#include "status.h"
#include "tune.h"

void Compute_Ignition()
{
	// Used fixed values for start
	if(status.flags.cranking)
	{
		status.output.ign_timing = (float)tune.cranking_advance / 10;
		status.output.ign_dwell = (float)tune.cranking_dwell / 1000000;
	}
	else	// Switch over to real values for run
	{
		// Get timing from table
		uint16_t timing = lut_table2d16_int16(&tune.ign, status.input.rpm, status.input.map);
		status.output.ign_timing = (float)timing / 10;
		// Get dwell from table
		status.output.ign_dwell = lut_table1d16(&tune.dwell, status.input.batt);
	}
}
