#include "can.h"

#include "stm32f37x.h"

#include "serial.h"

uint16_t ftou16(float f, float scale)
{
	return (uint16_t)((f * scale) + 0.5f);
}

uint8_t ftou8(float f, float scale)
{
	return (uint8_t)((f * scale) + 0.5f);
}

int16_t ftoi16(float f, float scale)
{
	return (int16_t)((f * scale) + 0.5f);
}

void Init_CAN()
{
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER |= GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1;
	GPIOA->AFR[1] |= 0x99 << 12;


	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;


	// Enter init mode
	CAN1->MCR |= CAN_MCR_INRQ;
	// Wait for init mode
	while ((CAN1->MSR & CAN_MSR_INAK) == 0);

	CAN1->BTR = (0 << 24) |	// SJW = 1
				(1 << 20) | // TS2 = 2
				(12 << 16) |	// TS1 = 13
				(3 << 0);   // BRP = 1/4

	CAN1->MCR |= CAN_MCR_NART;

	// Leave init mode
	CAN1->MCR &= ~CAN_MCR_INRQ;
	// Wait for end of init mode
	while (CAN1->MSR & CAN_MSR_INAK);

	// Leave sleep
	CAN1->MCR &= ~CAN_MCR_SLEEP;
	// Wait for the awakening
	while (CAN1->MSR & CAN_MSR_SLAK);




	CAN1->FMR |= CAN_FMR_FINIT;
	CAN1->sFilterRegister[0].FR1 = 0;
	CAN1->sFilterRegister[0].FR2 = 0;
	CAN1->FA1R = 1;
	CAN1->FMR &= ~CAN_FMR_FINIT;

	CAN1->IER |= CAN_IER_FMPIE0;

	NVIC_EnableIRQ(CAN1_RX0_IRQn);
}


#include "stdlib.h"

void CAN1_RX0_IRQHandler()
{
	if(CAN1->RF0R & CAN_RF0R_FMP0)
	{
		can_frame_t f;

		f.data32[1] = CAN1->sFIFOMailBox[0].RDHR;
		f.data32[0] = CAN1->sFIFOMailBox[0].RDLR;
		f.id = CAN1->sFIFOMailBox[0].RIR >> 21;

		CAN_RecieveFrame(f);

		// Release message
		CAN1->RF0R |= CAN_RF0R_RFOM0;
	}
}

// Recieves one can frame, either from real CAN, or CAN-over-USART
void CAN_RecieveFrame(can_frame_t frame)
{
	// Echo frame over serial
	Serial_SendCAN(frame);
}
