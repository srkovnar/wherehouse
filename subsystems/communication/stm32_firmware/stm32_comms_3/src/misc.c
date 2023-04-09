#include <misc.h>


/*
 * Converts an integer to a char * representation
 */
int dtostr(signed int d, char * out, int out_len)
{
	if (out_len < 12) return -1 ;

	int o_i = 0;
	int zero = 0;

	if (d < 0)
	{
		out[o_i] = '-';
		d = d * -1;
		o_i ++;
	}

	if (d == 0)
	{
		out[o_i] = '0';
		o_i++;
		out[o_i] = '\0';
		return o_i;
	}

	for (int i = UNSIGNED_INT_RAD; i > 0; i /= 10)
	{
		if (zero == 1 || d/i != 0)
		{
			out[o_i] = (char)(48 + d/i);
			zero = 1;
			o_i++;
		}
		d -= (d/i)*i;
	}


	out[o_i] = '\0';

	return o_i;
}


static void nanoWait(unsigned int n)
{
	asm(    "        mov r0,%0\n"
			"repeat: sub r0,#83\n"
			"        bgt repeat\n" : : "r"(n) : "r0", "cc");
}
