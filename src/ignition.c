#include "status.h"
#include "tune.h"

void Compute_Ignition()
{
	// Used fixed values for start
	if(status.flags.cranking)
	{
		status.output.ign_timing = tune.cranking_advance;
		status.output.ign_dwell = tune.cranking_dwell;
	}
	else	// Switch over to real values for run
	{
		// Get timing from table
		status.output.ign_timing = lut_table2d16(&tune.ign, status.input.rpm, status.input.map);
		// Get dwell from table
		status.output.ign_dwell = lut_table1d16(&tune.dwell, status.input.batt);
	}
}
