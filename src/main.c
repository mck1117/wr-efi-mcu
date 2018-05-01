#include "main.h"

#include "can.h"
#include "serial.h"

int main(void)
{
	Init_Serial();

	Init_CAN();

	Serial_SendStr("Hello, world!");

	// loop da loop
	while(1);
}
