

#include "fpga.h"

#include "stm32f37x.h"

#include "tune.h"
#include "status.h"

void FPGA_WriteReg(uint8_t addr, uint16_t value)
{
	addr = (addr << 1) | 0x01;	// Address is high 7 bits, write enable is LSB

	uint8_t val_low = value & 0xFF;
	uint8_t val_high = value >> 8;

	// Wait for not busy
	while(SPI2->SR & SPI_SR_BSY);

	SPI2->DR = addr;
	SPI2->DR = val_high;
	SPI2->DR = val_low;

	// Wait for completion
	while(SPI2->SR & SPI_SR_BSY);

	// Burn recv'd bytes
	uint8_t v = SPI2->DR;
	v = SPI2->DR;
	v = SPI2->DR;
}

uint16_t FPGA_ReadReg(uint8_t addr)
{
	addr = addr << 1;

	// Wait for not busy
	while(SPI2->SR & SPI_SR_BSY);


	SPI2->DR = addr;
	SPI2->DR = 0;		// Two dummy bytes to read back data
	SPI2->DR = 0;

	// Wait for completion
	while(SPI2->SR & SPI_SR_BSY);

	// Burn a byte
	uint8_t v = SPI2->DR;

	// Read high/low bytes
	uint8_t high = SPI2->DR;
	uint8_t low = SPI2->DR;

	// Return combined 16 bit value
	return high << 8 | low;
}


void Init_FPGA_SPI()
{
	// Set up SPI2 to talk to the FPGA
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

	SPI2->CR1 = SPI_CR1_SPE | (7 << 3) | SPI_CR1_MSTR;	// Enable, master, clock /256
	SPI2->CR2 = (7 << 8);	// 8 bit data size
}

void Init_FPGA_GPIO()
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIODEN;

	// Set GPIO B af
	GPIOB->MODER &= ~(GPIO_MODER_MODER14 | GPIO_MODER_MODER15);
	GPIOB->MODER |= GPIO_MODER_MODER14_1 | GPIO_MODER_MODER15_1;
	GPIOB->AFR[1] |= 0x55000000;	// SPI2 is af5 (miso & mosi)

	// Set GPIO D af
	GPIOD->MODER &= ~GPIO_MODER_MODER8;
	GPIOD->MODER |= GPIO_MODER_MODER8_1;
	GPIOD->AFR[1] |= 0x00000005;	// SPI2 is af5 (sck)
}

void Init_FPGA_Config_Timing()
{
	FPGA_WriteReg(FPGA_REG_TOOTH_COUNT, tune.engine.tooth_count);
	FPGA_WriteReg(FPGA_REG_TOOTH_WIDTH, tune.engine.tooth_width);
	FPGA_WriteReg(FPGA_REG_TEETH_MISSING, tune.engine.teeth_missing);
	FPGA_WriteReg(FPGA_REG_QUANTA_PER_REV, tune.engine.quanta_per_rev);

	// Write 4 spark output phases
	for(int i = 0; i < 4; i++)
	{
		FPGA_WriteReg(FPGA_REG_CYL_PHASE + i, tune.engine.cyl_phase[i]);
	}
}

void Init_FPGA()
{
	Init_FPGA_SPI();
	Init_FPGA_GPIO();

	Init_FPGA_Config_Timing();
}

void FPGA_Read()
{
	status.input.rpm = 1234;
	status.flags.synced = 1;
}

void FPGA_WriteRun()
{
	// Figure out ignition timing
	float timing = status.output.ign_timing;

	float timing_actual = 360 - timing;

	// Correct wrap (because the FPGA doesn't)
	while(timing_actual >= 360) timing_actual -= 360;
	while(timing_actual < 0) timing_actual += 360;

	// Convert from engine degrees to engine sync quanta
	uint16_t timing_quanta = (uint16_t)(tune.engine.quanta_per_rev * (timing_actual / 360));

	// Convert from dwell period (seconds) to sync quanta
	uint16_t dwell_quanta = (uint16_t)(status.output.ign_dwell / status.computations.period * tune.engine.quanta_per_rev);

	// Convert from pulse width period (seconds) to microseconds
	uint16_t inj_pw = (uint16_t)(status.output.injector_pw * 1000000);

	FPGA_WriteReg(FPGA_REG_TIMING, timing_quanta);
	FPGA_WriteReg(FPGA_REG_DWELL, dwell_quanta);

	FPGA_WriteReg(FPGA_REG_INJ_PW1, inj_pw);
	FPGA_WriteReg(FPGA_REG_INJ_PW2, 0);
}
