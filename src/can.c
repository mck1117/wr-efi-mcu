#include "can.h"

#include "status.h"
#include "stm32f37x.h"

#include "status.h"
#include "timers.h"

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

void can_frame_rpm_map(can_frame_t* f)
{
	f->id = CAN_ID_RPM_MAP;

	// Round RPM to nearest (not truncate)
	f->data16[0] = ftou16(status.input.rpm, 1);
	//f->data16[1] = (uint16_t)status.flags;
	f->data16[1] = 0;
	f->data16[2] = ftou16(status.input.map, 10);
	f->data16[3] = ftoi16(status.input.tps, 10);
}

void can_frame_sens(can_frame_t* f)
{
	f->id = CAN_ID_SENS;

	f->data16[0] = ftoi16(status.input.clt, 10);
	f->data16[1] = ftoi16(status.input.iat, 10);
	f->data16[2] = ftou16(status.input.afr1, 100);
	f->data16[3] = ftou16(status.input.afr2, 100);
}

void can_frame_system(can_frame_t* f)
{
	f->id = CAN_ID_SYSTEM;

	f->data8[0] = status.system.cpu_usage;
	f->data8[1] = ftou8(status.system.cpu_temp + 40, 1);
	f->data16[1] = ftou16(status.input.batt, 100);
	f->data32[1] = system_timer;	// runtime in milliseconds
}

void can_frame_calcs_fuel(can_frame_t* f)
{
	f->id = CAN_ID_CALCS_FUEL;

	f->data16[0] = ftou16(status.output.injector_pw, 1000000);	// Output integer microseconds

	f->data8[2] = ftou8(status.computations.gamma, 100);	// Output in percent (0.9 gamma = 90 value)
	f->data8[3] = ftou8(status.computations.ve, 100);
	f->data8[4] = ftou8(status.computations.lambda_correction, 100);
	f->data8[5] = ftou8(status.computations.injector_duty, 100);

	f->data16[3] = ftou8(status.computations.afr_target, 100);
}

void can_frame_calcs_ign(can_frame_t* f)
{
	f->id = CAN_ID_CALCS_IGN;

	f->data16[0] = ftoi16(status.output.ign_timing, 10);
	f->data16[1] = ftoi16(status.output.ign_dwell, 1000000);	// output integer microseconds
	f->data16[2] = 0;
	f->data16[3] = 0;
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

	CAN1->BTR = (1 << 24) |	// SJW = 1
				(1 << 20) | // TS2 = 2
				(4 << 16) |	// TS1 = 5
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
	CPU_busy_int();

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

	CPU_idle_int();
}

#include "can_tuning.h"

// Recieves one can frame, either from real CAN, or CAN-over-USART
void CAN_RecieveFrame(can_frame_t frame)
{
	if(frame.id == 0x100)
	{
		CAN_Tuning_ProcessFrame(frame);
	}

	// Echo frame over serial
	//Serial_SendCAN(frame);
}


static can_frame_t fifo[CAN_FIFO_LENGTH];
static uint32_t fifo_w = 0;
static uint32_t fifo_r = 0;


static void CAN_AddFifo(CanFrameGenerator_t generatorFunc)
{
	can_frame_t* frame = &fifo[fifo_w];

	// Fill the frame
	generatorFunc(frame);

	frame->notSent = 1;

	if(fifo_w == CAN_FIFO_LENGTH - 1)
	{
		fifo_w = 0;
	}
	else
	{
		fifo_w++;
	}
}

static void CAN_FillMailboxes()
{
	// While there is an empty mailbox, and there is a message in the FIFO to send
	while(CAN1->TSR & CAN_TSR_TME && fifo[fifo_r].notSent)
	{
		// Get an empty mailbox index
		uint8_t mailbox = (CAN1->TSR & CAN_TSR_CODE) >> 24;

		can_frame_t* f = &fifo[fifo_r];


		// Fill the mailbox data
		CAN1->sTxMailBox[mailbox].TDHR = f->data32[1];
		CAN1->sTxMailBox[mailbox].TDLR = f->data32[0];
		CAN1->sTxMailBox[mailbox].TIR = f->id << 21;
		CAN1->sTxMailBox[mailbox].TDTR = 8;

		// Fire the transmission
		CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;

		// Mark the frame as sent
		f->notSent = 0;

		// Move FIFO pointer
		if(fifo_r == CAN_FIFO_LENGTH - 1)
		{
			fifo_r = 0;
		}
		else
		{
			fifo_r++;
		}
	}
}


CanFrameGenerator_t frames_50hz[] = {
		can_frame_rpm_map,
		can_frame_sens,
		can_frame_system,
		can_frame_calcs_fuel,
		can_frame_calcs_ign,
};


static void CAN_Enqueue50hz()
{
	int count = sizeof(frames_50hz) / sizeof(CanFrameGenerator_t);

	// Push everyone in to the FIFO
	for(int i = 0; i < count; i++)
	{
		CAN_AddFifo(frames_50hz[i]);
	}
}

void CAN_Transmits()
{
	static uint32_t i = 0;

	if(i == 1000)
	{
		i = 0;
	}

	// At 50hz...
	if(i % 20 == 0)
	{
		CAN_Enqueue50hz();
	}

	// Attempt to empty out the FIFO in to the mailboxes
	CAN_FillMailboxes();

	i++;
}


