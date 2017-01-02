

#include "stm32f37x.h"

#include "serial.h"

static void Init_Serial_GPIO()
{
	// Turn on GPIO B
	// USART3 is on b8/b9
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	// Clear existing mode(s)
	GPIOB->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9);
	// Mode = alt func
	GPIOB->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1;

	// Clear existing alt funcs
	GPIOB->AFR[1] &= ~(GPIO_AFRH_AFRH0 | GPIO_AFRH_AFRH1);
	// Set alt funcs, USART3 is af7
	GPIOB->AFR[1] |= (7 << 0) | (7 << 4);
}

static void Init_Serial_USART()
{
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

	USART3->CR1 = USART_CR1_RXNEIE | USART_CR1_TE |	// Enable tx, enable rx interrupt
				  USART_CR1_RE;						// Enable rx
	USART3->CR2 = 0;
	USART3->CR3 = USART_CR3_DMAT;	// Enable DMA for transmit
	USART3->BRR = 64;			// 500k baud
	USART3->ICR = 0xFFFFFFFF;	// Clear all interrupts
}

static uint8_t send_buffer[128];

static void Init_Serial_DMA()
{
	// Turn on DMA controller
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;

	// Just set peripheral addr for now, the rest changes later
	DMA1_Channel2->CCR = 0;
	DMA1_Channel2->CPAR = (uint32_t)&USART3->TDR;
}



void Init_Serial()
{
	Init_Serial_USART();
	Init_Serial_GPIO();
	Init_Serial_DMA();

	// Enable USART3 interrupts
	//NVIC_EnableIRQ(USART3_IRQn);

	// Lastly, enable USART3
	USART3->CR1 |= USART_CR1_UE;
}

#include <string.h>



void Serial_Send(const uint8_t* data, uint16_t length)
{
	memcpy(send_buffer, data, length);

	DMA1->IFCR |= DMA_IFCR_CGIF2 | DMA_IFCR_CTCIF2 | DMA_IFCR_CHTIF2;

	DMA1_Channel2->CCR = DMA_CCR_MINC | DMA_CCR_DIR;
	DMA1_Channel2->CMAR = (uint32_t)send_buffer;
	DMA1_Channel2->CNDTR = length;

	// Start the transfer
	DMA1_Channel2->CCR |= DMA_CCR_EN;
}

void Serial_SendStr(const char* str)
{
	uint16_t len = strlen(str);

	Serial_Send((uint8_t*)str, len);
}

char nibtoa(uint8_t nibble)
{
	if(nibble >= 0xA)
	{
		return nibble - 0xA + 'a';
	}
	else
	{
		return nibble + '0';
	}
}

void u8toa(uint8_t value, char* str)
{
	uint8_t hi = value >> 4;
	uint8_t lo = value & 0x0F;

	*str = nibtoa(hi);
	*(str + 1) = nibtoa(lo);
}

void u16toa(uint16_t value, char* str)
{
	uint8_t hi = value >> 8;
	uint8_t lo = value & 0xFF;

	// A uint16 is just two uint8s
	u8toa(hi, str);
	u8toa(lo, str + 2);
}

#include "can.h"

void Serial_SendCAN(can_frame_t frame)
{
	char buf[23];

	u16toa(frame.id, buf);

	*(buf + 4) = ':';

	for(int i = 0; i < 8; i++)
	{
		int idx_base = i * 2 + 5;

		u8toa(frame.data8[i], buf + idx_base + 1);
	}

	*(buf + 21) = '\r';
	*(buf + 22) = '\n';

	Serial_Send((uint8_t*)buf, 23);
}
