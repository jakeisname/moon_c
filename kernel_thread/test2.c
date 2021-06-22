#include <stdio.h>

int main()
{
	volatile long x = 0;

	while (1) {
		x++;

		if (!(x % 100000000))
			printf("x=%ld\n", x);
	}
}

