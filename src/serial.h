#ifndef __SERIAL_H
#define __SERIAL_H

#include "stdint.h"

void Init_Serial();



void Serial_Send(const uint8_t* data, uint16_t length);
void Serial_SendStr(const char* str);



#endif
