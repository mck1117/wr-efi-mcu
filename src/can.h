
#ifndef __CAN_H
#define __CAN_H

#include "stdint.h"

#define CAN_ID_RPM_MAP 		0x10
#define CAN_ID_SENS 		0x11
#define CAN_ID_SYSTEM		0x12
#define CAN_ID_CALCS_FUEL	0x13
#define CAN_ID_CALCS_IGN	0x14

#define CAN_FIFO_LENGTH 16

typedef struct can_frame_s {
	uint16_t id;
	uint16_t res0;
	uint8_t notSent;

	union
	{
		uint8_t data8[8];
		uint16_t data16[4];
		uint32_t data32[2];
		float dataf[2];
	};
} can_frame_t;

typedef void (*CanFrameGenerator_t)(can_frame_t*);

void Init_CAN();
void CAN_Transmits();

void CAN_RecieveFrame(can_frame_t frame);

#endif
