

#include "fpga.h"

#include "stm32f37x.h"

#include "tune.h"
#include "status.h"

uint8_t SPI_DR8r(SPI_TypeDef* spi)
{
	return *(uint8_t*)(&spi->DR);
}

void SPI_DR8w(SPI_TypeDef* spi, uint8_t val)
{
	*(uint8_t*)(&spi->DR) = val;
}

void SPI_wait()
{
	uint32_t counter = 10;
	while(counter--);
}

void FPGA_WriteReg(uint8_t addr, uint16_t value)
{
	addr = (addr << 1) | 0x01;	// Address is high 7 bits, write enable is LSB

	uint8_t val_low = value & 0xFF;
	uint8_t val_high = value >> 8;

	// Wait for not busy
	while(SPI2->SR & SPI_SR_BSY);

	GPIOA->BSRR |= GPIO_BSRR_BR_8;	// CS low

	SPI_DR8w(SPI2, addr);
	SPI_DR8w(SPI2, val_high);
	SPI_DR8w(SPI2, val_low);

	// Wait for completion
	while(SPI2->SR & SPI_SR_BSY);

	//SPI_wait();



	// Burn recv'd bytes to clear FIFO
	SPI_DR8r(SPI2);
	SPI_DR8r(SPI2);
	SPI_DR8r(SPI2);

	GPIOA->BSRR |= GPIO_BSRR_BS_8;	// CS hi
}

uint16_t FPGA_ReadReg(uint8_t addr)
{
	addr = addr << 1;

	// Wait for not busy
	while(SPI2->SR & SPI_SR_BSY);

	GPIOA->BSRR |= GPIO_BSRR_BR_8;	// CS low

	SPI_DR8w(SPI2, addr);
	SPI_DR8w(SPI2, 0xBE);	// Two dummy bytes to read back data
	SPI_DR8w(SPI2, 0xEF);

	// Wait for completion
	while(SPI2->SR & SPI_SR_BSY);

	// Burn a byte
	SPI_DR8r(SPI2);

	// Read high/low bytes
	uint8_t high = SPI_DR8r(SPI2);
	uint8_t low = SPI_DR8r(SPI2);

	GPIOA->BSRR |= GPIO_BSRR_BS_8;	// CS hi

	// Return combined 16 bit value
	return high << 8 | low;
}


void Init_FPGA_SPI()
{
	// Set up SPI2 to talk to the FPGA
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

	SPI2->CR1 = (1 << 3) | SPI_CR1_MSTR | SPI_CR1_CPOL;	// Master, clock /4
	SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;	// ignore stupid nss pin
	SPI2->CR2 = (7 << 8);	// 8 bit data size

	SPI2->CR1 |= SPI_CR1_SPE;
}

void Init_FPGA_GPIO()
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOAEN;

	// Set GPIO A regular output
	GPIOA->BSRR |= GPIO_BSRR_BS_8;	// CS line idles hi
	GPIOA->MODER &= ~(GPIO_MODER_MODER8);
	GPIOA->MODER |= GPIO_MODER_MODER8_0;


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
	FPGA_WriteReg(FPGA_REG_TEETH_MISSING, tune.engine.teeth_missing);

	// Write 4 spark output phases
	for(int i = 0; i < 4; i++)
	{
		FPGA_WriteReg(FPGA_REG_CYL_PHASE_BASE + i, tune.engine.cyl_phase[i]);
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
	status.input.rpm = FPGA_ReadReg(FPGA_REG_RPM);
	uint8_t fpga_status = FPGA_ReadReg(FPGA_REG_STATUS);

	status.flags.synced = (fpga_status & 0x01) == 1;
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

	// If we're synced, enable everything
	if(status.flags.synced)
	{
		FPGA_WriteReg(FPGA_REG_ENABLES, 0x37);
	}
	else
	{
		FPGA_WriteReg(FPGA_REG_ENABLES, 0x00);
	}
}
