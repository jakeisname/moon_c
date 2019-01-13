
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_DUMP_PACKET

void dump_packet(char *tx_rx, char *p, int start, int len)
{
#ifdef DEBUG_DUMP_PACKET
	unsigned char buff[64];
	int i, pos;
	int pre = 0;
	unsigned char c;

	for (i = 0; i < len; i++) {
		pos = i % 16;
		c = *(p + i);

		if (!pos) {
			sprintf(buff, "[%s] 0x%-4x: ", tx_rx, start + i);
			pre = strlen(buff);		
		}

		sprintf(buff + pre + pos * 3, "%02x ", c);

		if ((pos == 15) || (i == len -1))
			printf("%s\n", buff);
	}		
#endif
}

