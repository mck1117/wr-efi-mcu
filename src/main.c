#include "main.h"

#include "adc.h"
#include "can.h"
#include "fpga.h"
#include "sensors.h"
#include "serial.h"
#include "timers.h"
#include "tune.h"

#include "status.h"

#include "fueling.h"
#include "ignition.h"

//#include <stdio.h>

volatile uint32_t system_timer = 0;
volatile int init_completed = 0;

int main(void)
{
	Load_Tune();

	Init_CPUUsage();
	Init_Serial();
	Init_Timers();
	Init_ADC();

	Init_CAN();

	Start_Timers();

	// Wait for FPGA to program
	while(system_timer < 1000) ;

	// Now we can init the FPGA
	Init_FPGA();

	// Load tune in to RAM from FLASH
	//Load_Tune();

	init_completed = 1;

	// loop da loop
	while(1);
}

/*
 * Everything that needs to happen at 1khz
 */
void Events_1khz()
{
	/*
	 * General plan:
	 * 1) Read FPGA status
	 * 2) calculate/smooth sensors
	 * 2b) If we aren't synced, update MAP averaging.
	 * 	   This has to happen because we normally average MAP
	 * 	   over the period of one cylinder, but if the engine
	 * 	   isn't spinning, this won't happen, but we still want
	 * 	   a MAP reading (but the averaging doesn't really matter,
	 * 	   since the engine isn't running and causing it to
	 * 	   fluctuate and need averaging).
	 * 3) Calculate fueling
	 * 4) Calculate ign
	 * 5) write outputs to FPGA
	 *
	 */

	system_timer++;
	// Update the temperature sensor
	ADC_UpdateTempSensor();

	CAN_Transmits();

	if(init_completed)
	{
		FPGA_Read();

		status.flags.synced = 0;

		// If we aren't synced, manually update the MAP averaging
		// (while synced this happens in the per-cyl interrupt)
		if(!status.flags.synced)
		{
			ADC_UpdateMapAverage();
		}

		// Update sensor smoothing and mappings
		Sensors_Update();

		// Compute fuel pulse width
		Compute_Fueling();

		// Compute ignition timing
		Compute_Ignition();

		// Write outputs to FPGA
		FPGA_WriteRun();
	}
}

/*
 * Everything that needs to happen per cylinder
 */
void Events_Sync()
{
	ADC_UpdateMapAverage();
}
