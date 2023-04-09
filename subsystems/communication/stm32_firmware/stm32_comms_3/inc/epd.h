#ifndef __EPD_H__
#define __EPD_H__

#include "stm32f0xx.h"


#define P_DC 	3
#define P_BUSY 	2
#define P_RST 	1

#define DISPLAY_WIDTH 176
#define DISPLAY_HIGHT 264

#define DISPLAY_BUFFER_WIDTH 22 // Determined as DISPLAY_WIDTH/8 See Display Documentation for Explaination

uint8_t display_buffer[DISPLAY_BUFFER_WIDTH][DISPLAY_HIGHT];

void EPD_setReg(uint8_t reg);
void EPD_sendByte(uint8_t data);

void EPD_init();

void EPD_sendBuffer();
void EPD_update();

void EPD_powerOff();


int EPD_drawPixel(int bw, int x, int y);
int EPD_clear(int bw);


#endif
