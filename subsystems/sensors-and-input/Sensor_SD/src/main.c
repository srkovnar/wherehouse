/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"



int main(void)
{
    uint32_t value;
    uint32_t fixer;
    uint32_t checker;
    int avg;
    avg = 0;
    fixer = 0x800000;
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
    int offset = 0;
    for(;;) {
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
                GPIOB->BSRR = GPIO_ODR_13;
                GPIOB->BRR = GPIO_ODR_13;

        }
        value = value ^ fixer;
        //    avg += value;
        //}
        //avg = avg / 100;
        //int show = value;
        int balance = (value / 15000) - offset;
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
    }
}

