/**
  ******************************************************************************
  * @file    main.chttps://share.snapchat.com/m/qLh6K834?share_id=o6_SMkXGgzs&locale=en-US
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include <display.h>
#include <misc.h>

volatile int display_update_flag = 1;
int previous_quantity = 0;
int quantity = 0;

void init_tim6();
void init_sensors();
void display_ui();

int main(void)
{
	uint32_t value;
	uint32_t fixer;
	uint32_t checker;
	int offset;
	int avg;
	int esp8266_weight = 154;
	int item_weight = 5;
	avg = 0;
	fixer = 0x800000;
	offset = 0;

	init_sensors();

	init_tim6();

	DISPLAY_init();



	for(;;)
	{
		offset = 0;
		checker = (GPIOB->IDR);
		while (GPIOB->IDR & (1<<14)){}
		//for (int x = 0; x < 100; x++){
			value = 0;
			for (int i = 0; i <24; i++){
				wait(5);
				GPIOB->BSRR = GPIO_ODR_13;
				value = value << 1;
				GPIOB->BRR = GPIO_ODR_13;
				if (GPIOB->IDR & (1<<14)){
					value += 1;
				}
				//GPIOB->BSRR = GPIO_ODR_13;
				//GPIOB->BRR = GPIO_ODR_13;

		}
		value = value ^ fixer;
		//    avg += value;
		//}
		//avg = avg / 100;
		//int show = value;
		int balance = (value / 15000) - offset;

		//Buttons
		if ((GPIOB->IDR & (1<<12))){GPIOC->BSRR = GPIO_ODR_6;}
		if (!(GPIOB->IDR & (1<<12))){GPIOC->BRR = GPIO_ODR_6;}
		int holder = GPIOB->IDR & (1<<12);
		while (!(holder & (1<<12))){
			if (!((GPIOB->IDR & (1<<11)))){
				 balance += 1;
			}
			if (!((GPIOB->IDR & (1<<11)))){
				balance -= 1;
			}
			if (!((GPIOB->IDR & (1<<9)))){
				balance = 0;
			}
			if (!((GPIOB->IDR & (1<<8)))){
				offset = value / 15000;
			}
			if (!((GPIOB->IDR & (1<<12)))){
				holder = (1<<12);
			}

		}
		//avg = 0;

		quantity =   ((value)/450 - 18480)/item_weight;
		if ( display_update_flag == 1)
		{

			display_ui();

			display_update_flag = 0;
			previous_quantity = quantity;
		}
	}
}

void display_ui()
{
	int failed = 0;
	char qty_str[12];

	DISPLAY_clear(0);

	DISPLAY_drawChar(10, 10, '2', 1);
	DISPLAY_drawRect(0, 0, 264, 176, 1);
	DISPLAY_drawRect(5, 5, 254, 166, 0);
	DISPLAY_drawRect(0, 126, 230, 50, 1);
	DISPLAY_drawRect(5, 131, 220, 40, 0);

	DISPLAY_print(9, 133, 1, "Shelf ID: \0");
	DISPLAY_print(9, 80, 1, "Item: 100mH \0");
	DISPLAY_print(9, 40, 1, "Quantity: \0");

	if ( (failed = dtostr(quantity, qty_str, 12) )  == -1 || failed == 0)
	{
		DISPLAY_clear(0);
		DISPLAY_print(4, 40, 1, "ERR: Failed to get string rep\0");
	}
	else
	{
		//DISPLAY_clear(0);
		DISPLAY_print(160,40, 1, qty_str);
		//DISPLAY_print(20,150, 1, qty_str);
		//dtostr(123, qty_str, 12);
		//DISPLAY_print(5,30, 1, qty_str);
		//dtostr(1234, qty_str, 12);
		//DISPLAY_print(5,60, 1, qty_str);
		//dtostr(12345, qty_str, 12);
		//DISPLAY_print(5,90, 1, qty_str);
		//dtostr(123456, qty_str, 12);
		//DISPLAY_print(5,120, 1, qty_str);
		//dtostr(0, qty_str, 12);
		//DISPLAY_print(5,150, 1, qty_str);
	}

	DISPLAY_update();
}

void init_tim6()
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

	NVIC->ISER[0] |= 0x1 << 17;

	TIM6->PSC = 48000 - 1;
	TIM6->ARR = 1000 - 1;
	TIM6->DIER |= 0x1;

	TIM6->CR1 |= TIM_CR1_CEN;
}

void TIM6_DAC_IRQHandler ()
{
	TIM6->SR &= ~0x1;
	if (quantity != previous_quantity)
	{
		display_update_flag = 1;
	}
}

void init_sensors()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOB->MODER |= 0<<(2*14);
    GPIOB->ODR |= 1<<14;
    GPIOB->MODER |= 0<<(2*12);
    GPIOB->ODR |= 1<<12;
    GPIOB->MODER |= 0<<(2*11);
    GPIOB->ODR |= 1<<11;
    GPIOB->MODER |= 0<<(2*10);
    GPIOB->ODR |= 1<<10;
    GPIOB->MODER |= 0<<(2*9);
    GPIOB->ODR |= 1<<9;
    GPIOB->MODER |= 0<<(2*8);
    GPIOB->ODR |= 1<<8;
    GPIOB->MODER &= ~(3<<(2*13));
    GPIOB->MODER |= 1<<(2*13);
    GPIOC->MODER |= 1<<(2*6);
    GPIOB->BRR = GPIO_ODR_13;
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFRH4 | GPIO_AFRH_AFRH5 | GPIO_AFRH_AFRH7);
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    SPI2->CR1 |= SPI_CR1_BIDIOE | SPI_CR1_BIDIMODE | SPI_CR1_MSTR | 0x38;   //SPI_CR1_BR;
    SPI2->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);
    SPI2->CR2 = SPI_CR2_DS_3 | SPI_CR2_DS_0 | SPI_CR2_SSOE | SPI_CR2_NSSP;
    SPI2->CR1 |= SPI_CR1_SPE;
}
