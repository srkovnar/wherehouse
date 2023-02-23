#include <epd.h>


void setupGpio();
void setupSpi();

static void nano_wait(unsigned int n) {
	asm(    "        mov r0,%0\n"
			"repeat: sub r0,#83\n"
			"        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void EPD_init()
{
	setupGpio();
	setupSpi();

	// Reset Display

	/*
	 * Reset CoG Driver
	 */

	// Strobe RST Pin
	nano_wait(5000); //5ms delay
	GPIOA->BSRR = 0x1 << P_RST;
	nano_wait(5000); //5ms delay
	GPIOA->BRR = 0x1 << P_RST;
	nano_wait(10000);	// 10ms delay
	GPIOA->BSRR = 0x1 << P_RST;
	nano_wait(5000); //5ms delay

	// Soft reset
	// Write to Register 0x00 data 0x0E
	EPD_setReg(0x00);
	EPD_sendByte(0x0E);

	/*
	 * Set Temperature and PSR
	 */

	// assume 25 degrees 0x19
	// Write to Register 0xe5 value 0x19
	// Write to Register 0xe0 value 0x2
	EPD_setReg(0xe5);
	EPD_sendByte(0x19);

	EPD_setReg(0xe0);
	EPD_sendByte(0x2);

	// Set PSR Value
	// Write Register 0x00 value 0xCF 0x8D
	EPD_setReg(0x00);
	EPD_sendByte(0xcf);
	EPD_sendByte(0x8d);
}

void setupGpio()
{
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// SPI GPIO
	GPIOA->MODER &= ~0x0000ff00;
	GPIOA->MODER |= 0x0000aa00;

	// GPIO For Controlling the Display
	GPIOA->MODER &= ~0x000000fe;
	GPIOA->MODER |= 0x00000044;
}

void setupSpi()
{
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	GPIOA->AFR[0] &= ~0xffff0000;

	SPI1->CR1 &= ~SPI_CR1_SPE;
	SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;
	SPI1->CR2 &= ~SPI_CR2_DS_3;
	SPI1->CR2 |= SPI_CR2_SSOE;
	SPI1->CR2 |= SPI_CR2_NSSP;
	SPI1->CR1 |= SPI_CR1_BR_1;
	SPI1->CR1 |= SPI_CR1_MSTR;
	SPI1->CR1 |= SPI_CR1_SPE;
}

void EPD_setReg(uint8_t reg)
{
	while (SPI1->SR & SPI_SR_BSY);
	GPIOA->BRR = 0x1 << P_DC;

	* ((uint8_t *) & SPI1->DR) = reg;

	while (SPI1->SR & SPI_SR_BSY);
	GPIOA->BSRR = 0x1 << P_DC;
}

void EPD_sendByte(uint8_t data)
{
	while (SPI1->SR & SPI_SR_BSY);
	GPIOA->BSRR = 0x1 << P_DC;

	* ((uint8_t *) & SPI1->DR) = data;
}


void EPD_sendBuffer()
{
	// First Frame
	EPD_setReg(0x10);

	for (int i = 0; i < DISPLAY_HIGHT; i++)
	{
		for (int j = 0; j < DISPLAY_BUFFER_WIDTH; j++)
		{
			EPD_sendByte(display_buffer[j][i]);
		}
	}

	// Second Frame
	EPD_setReg(0x13);

	for (int i = 0; i < DISPLAY_HIGHT; i++)
	{
		for (int j = 0; j < DISPLAY_BUFFER_WIDTH; j++)
		{
			EPD_sendByte(0x00);
		}
	}
}


void EPD_update()
{

	while ((GPIOA->IDR & 0x1<<P_BUSY)>>P_BUSY != 1);

	EPD_setReg(0x04);
	EPD_setReg(0x04);

	while ((GPIOA->IDR & 0x1<<P_BUSY)>>P_BUSY != 1);

	EPD_setReg(0x12);
	EPD_setReg(0x12);

	while ((GPIOA->IDR & 0x1<<P_BUSY)>>P_BUSY != 1);
}

/*
 * TODO: power off chip in EPD to save power
 */
void EPD_powerOff()
{
	// Write This Later
	// For Saving Power
}

/*
 * Draw a Pixel on the screen
 * Note: The default orientation of the display is portrait mode
 * 		 this function flips it such that the orgin can be considered the bottom left corner
 * 		 however physically the orgin is the same
 */
int EPD_drawPixel(int bw, int x, int y)
{
	if (y >= DISPLAY_WIDTH || x >= DISPLAY_HIGHT || x < 0 || y < 0) return -1;

	int buffer_byte = y/8;
	int byte_offset = y%8;

	if (bw == 0)
	{
		display_buffer[buffer_byte][x] &= ~(0x80>>byte_offset);
	}
	else
	{
		display_buffer[buffer_byte][x] |= (0x80>>byte_offset);
	}

	return 0;
}

int EPD_clear(int bw)
{
	for (int i = 0; i < DISPLAY_HIGHT; i++)
	{
		for (int j = 0; j < DISPLAY_BUFFER_WIDTH; j++)
		{
			if (bw == 0)
			{
				display_buffer[j][i] = 0x00;
			}
			else
			{
				display_buffer[j][i] = 0xff;
			}
		}
	}
}
