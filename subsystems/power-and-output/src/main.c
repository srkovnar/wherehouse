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
#include <display.h>


			
static void nano_wait(unsigned int n) {
	asm(    "        mov r0,%0\n"
			"repeat: sub r0,#83\n"
			"        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

int main(void)
{

	DISPLAY_init();

	//DISPLAY_drawChar(10, 10, '2', 1);
	DISPLAY_drawRect(0, 0, 264, 176, 1);
	DISPLAY_drawRect(5, 5, 254, 166, 0);
	DISPLAY_drawRect(0, 126, 230, 50, 1);
	DISPLAY_drawRect(5, 131, 220, 40, 0);

	DISPLAY_print(9, 133, 1, "Shelf ID: \0");
	DISPLAY_print(9, 80, 1, "Item Type: \0");
	DISPLAY_print(9, 40, 1, "Item Quantity: \0");
	DISPLAY_update();

	for(;;);
}
