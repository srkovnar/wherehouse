#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <epd.h>

void DISPLAY_init();
void DISPLAY_clear(int bw);
void DISPLAY_update();

// draw primitives
void DISPLAY_drawRect(int x, int y, int w, int h, int bw);
void DISPLAY_drawLine(int x, int y, int x2, int y2, int w, int bw);
void DISPLAY_drawCircle(int x, int y, int r, int bw);

int DISPLAY_drawBitmap(int x, int y, uint8_t bitmap[], int len);
void DISPLAY_drawChar(int x, int y, char c, int bw);
void DISPLAY_print(int x, int y, int bw, char * str);

#endif
