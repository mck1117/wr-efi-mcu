

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
	USART3->BRR = 36;			// 2 mbit/s
	USART3->ICR = 0xFFFFFFFF;	// Clear all interrupts
}

static uint8_t send_buffer[1024];

static void Init_Serial_DMA()
{
	// Turn on DMA controller
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;

	// Just set peripheral addr for now, the rest changes later
	DMA1_Channel2->CCR = 0;
	DMA1_Channel2->CPAR = &USART3->TDR;
}



void Init_Serial()
{
	Init_Serial_USART();
	Init_Serial_GPIO();
	Init_Serial_DMA();

	// Enable USART3 interrupts
	NVIC_EnableIRQ(USART3_IRQn);

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
