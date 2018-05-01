#ifndef __SERIAL_H
#define __SERIAL_H

#include "stdint.h"

void Init_Serial();

#include "can.h"

void Serial_Send(const uint8_t* data, uint16_t length);
void Serial_SendStr(const char* str);
void Serial_SendCAN(can_frame_t frame);



#endif
