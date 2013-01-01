#include <linux/export.h>
/*
 Heavily based on GCC code.
 */
static unsigned long udivmodsi4(unsigned long num, unsigned long den, int modwanted)
{
	unsigned long bit = 1;
	unsigned long res = 0;

	while (den < num && bit && !(den & (1L<<31)))
	{
		den <<=1;
		bit <<=1;
	}
	while (bit)
	{
		if (num >= den)
		{
			num -= den;
			res |= bit;
		}
		bit >>=1;
		den >>=1;
	}
	if (modwanted) return num;
	return res;
}

long __udivsi3 (long a, long b)
{
	return udivmodsi4 (a, b, 0);
}
EXPORT_SYMBOL(__udivsi3);

long __umodsi3 (long a, long b)
{
	return udivmodsi4 (a, b, 1);
}
EXPORT_SYMBOL(__umodsi3);

