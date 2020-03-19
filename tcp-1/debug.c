
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump_packet(char *tx_rx, char *p, int start, int len)
{
	unsigned char buff[64];
	int i, pos;
	int pre = 0;
	unsigned char c;
	int len2 = len > 64 ? 64 : len;

	for (i = 0; i < len2; i++) {
		pos = i % 16;
		c = *(p + i);

		if (!pos) {
			sprintf(buff, "[%s] 0x%-4x: ", tx_rx, start + i);
			pre = strlen(buff);		
		}

		sprintf(buff + pre + pos * 3, "%02x ", c);

		if ((pos == 15) || (i == (len2 - 1)))
			printf("%s\n", buff);
	}		
}

