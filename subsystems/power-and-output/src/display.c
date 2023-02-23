#include <display.h>
#include <epd.h>
#include <font.h>

int mapChar(char c);

void DISPLAY_init()
{
	EPD_init();

	EPD_clear(0);

	//EPD_sendBuffer();

	//EPD_update();
	//EPD_powerOff();
}

/*
 * This function only exists to interface with the low level EPD Functions
 */
void DISPLAY_clear(int bw)
{
	EPD_clear(bw);
}


void DISPLAY_update()
{
	EPD_sendBuffer();

	EPD_update();
	EPD_powerOff();
}


void DISPLAY_drawRect(int x, int y, int w, int h, int bw)
{
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			if (EPD_drawPixel(bw, i+x, j+y) == -1)
				break;
		}
	}
}

/*
 * DISPLAY_drawBitmap
 * Draw a bitmap image on the display
 * Only supports microsoft bmp format in 1 bit color without compression
 */
int DISPLAY_drawBitmap(int x, int y, uint8_t bitmap[], int len)
{
	// Parse header
	// Process bitmap file header
	if (bitmap[0] != 0x42 || bitmap[1] != 0x4D || len < 14)
	{
		// Not a bitmap
		return -1;
	}

	//int filesize = bitmap[5]<<24 | bitmap[4]<<16 | bitmap[3]<<8 | bitmap[2];

	int data_start = bitmap[13]<<24 | bitmap[12]<<16 | bitmap[11]<<8 | bitmap[10];

	// Process DIB Header
	//int dib_headersize = bitmap[17]<<24 | bitmap[16]<<16 | bitmap[15]<<8 | bitmap[14];
	int bmp_width = bitmap[21]<<24 | bitmap[20]<<16 | bitmap[19]<<8 | bitmap[18];
	int bmp_hight = bitmap[25]<<24 | bitmap[24]<<16 | bitmap[23]<<8 | bitmap[22];

	int color_panels = bitmap[27]<<8 | bitmap[26];
	int bit_per_pixel = bitmap[29]<<8 | bitmap[28];	// Values other than 1 are not supported
	if (color_panels != 0x1 || bit_per_pixel != 0x1)
	{
		return -1;
	}

	int compression = bitmap[33]<<24 | bitmap[32]<<16 | bitmap[31]<<8 | bitmap[30];
	if (compression != 0x0)	// Image compression not supported
	{
		return -1;
	}

	int image_size = bitmap[37]<<24 | bitmap[36]<<16 | bitmap[35]<<8 | bitmap[34];

	/*
	 * Rest of the Header is unimportant
	 */

	int data_end = data_start + image_size;

	int row_len = 4 * (((bmp_width * bit_per_pixel) + 31) / 32);

	int dis_x = 0;
	int dis_y = 0;
	int px_value = 0;
	uint8_t img_byte = 0x00;
	for (int bmp_index = data_start; bmp_index < data_end; bmp_index++)
	{
		img_byte = bitmap[bmp_index];

		for (int k = 0; k < 8; k++)
		{
			px_value = (0x1 & ~(img_byte >> (7-k)));
			if (EPD_drawPixel(px_value, (dis_x++)+x, dis_y+y) == -1) break;
		}



		if (dis_x >= bmp_width)
		{
			bmp_index += (row_len - (bmp_width/8));
			dis_x = 0;
			dis_y++;
		}
	}

	return 0;
}

void DISPLAY_print(int x, int y, int bw, char * str)
{
	int i = 0;

	while (str[i] != '\0')
	{
		DISPLAY_drawChar((i)* (FONT_WIDTH + 2) + x, y, str[i], bw);
		i++;
	}
}

void DISPLAY_drawChar(int x, int y, char c, int bw)
{
	// Parse header

	//int filesize = bitmap[5]<<24 | bitmap[4]<<16 | bitmap[3]<<8 | bitmap[2];

	int data_start = f_consolas_18pt[13]<<24 | f_consolas_18pt[12]<<16 | f_consolas_18pt[11]<<8 | f_consolas_18pt[10];

	int bmp_width = f_consolas_18pt[21]<<24 | f_consolas_18pt[20]<<16 | f_consolas_18pt[19]<<8 | f_consolas_18pt[18];
	//int bmp_hight = f_consolas_18pt[25]<<24 | f_consolas_18pt[24]<<16 | f_consolas_18pt[23]<<8 | f_consolas_18pt[22];

	// Process DIB Header
	// int image_size = f_consolas_18pt[37]<<24 | f_consolas_18pt[36]<<16 | f_consolas_18pt[35]<<8 | f_consolas_18pt[34];

	int char_map = mapChar(c);

	int bmp_x = (char_map % FONT_COLUMNS) * FONT_WIDTH;
	int bmp_y = (char_map / FONT_COLUMNS) * FONT_HIGHT - (char_map / FONT_COLUMNS); // Hacky Fix to adjust the drift based on the hight
	int row_len = 4 * (((bmp_width) + 31) / 32);

	int start_byte = bmp_x/8 + bmp_y*row_len + data_start;
	int start_bit  = bmp_x % 8;

	int i = 0;	// iterator through x
	int j = 0;	// iterator through y
	int data_index_byte = start_byte;
	int data_index_bit  = start_bit;
	int px_value = 0;
	while (j < FONT_HIGHT)
	{
		for (int k = data_index_bit; k < 8 && i < FONT_WIDTH; k++)
		{
			px_value = (0x1 & ~(f_consolas_18pt[data_index_byte] >> (7-k)));
			if (EPD_drawPixel(px_value, (i++)+x, j+y) == -1) break;
		}
		data_index_bit = 0;
		data_index_byte++;
		if (i == FONT_WIDTH)
		{
			data_index_bit  = start_bit;
			j++;
			i = 0;
			data_index_byte = start_byte + j*row_len;
		}
	}

}

/*
 * maps Character to location on character bitmap
 */
int mapChar(char c)
{
	int index = 0;
	if (c >= 0x6C && c <= 0x7e)
	{
		index = ((int)c - 0x6C);
	}else if (c >= 0x59 && c <= 0x6B)
	{
		index = ((int)c -  0x59) + 19;
	}
	else if (c >= 0x46 && c <= 0x58)
	{
		index = ((int)c -  0x46) + 38;
	}else if (c >= 0x33 && c <= 0x45)
	{
		index = ((int)c -  0x33) + 57;
	}else if (c >= 0x20 && c <= 0x32)
	{
		index = ((int)c -  0x20) + 76;
	}
	else return -1;

	return index;
}
