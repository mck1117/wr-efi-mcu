#include "main.h"

#include "adc.h"
#include "fpga.h"
#include "sensors.h"
#include "serial.h"
#include "timers.h"
#include "tune.h"

#include "status.h"

#include "fueling.h"
#include "ignition.h"

int main(void)
{
	Init_Serial();
	Init_Timers();
	Init_ADC();

	Init_FPGA();

	// Load tune in to RAM from FLASH
	Load_Tune();

	// Fire this last
	Start_Timers();

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

	FPGA_Read();

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

	if(i == 100)
	{
		uint32_t cnt = TIM2->CNT;
		TIM2->CNT = 0;

		uint32_t oneSecond = 6400000;

		float usage = (float)cnt / oneSecond;

		char buf[50];
		sprintf(buf, "usage: %f\r\n", 1 - usage);
		Serial_SendStr(buf);

		i = 0;
	}

	i++;
}

/*
 * Everything that needs to happen per cylinder
 */
void Events_Sync()
{
	ADC_UpdateMapAverage();
}
