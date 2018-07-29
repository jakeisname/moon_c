#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/lp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "foo.h"

#define DEV_FILE "/dev/foo"

int main( int argc, char ** argv)
{
	int fd;
	int prnstate;
	int lp;
	struct foo_data data;

	unsigned char buff[128];

	fd = open(DEV_FILE, O_RDWR);
	if (fd < 0) {
		printf("open %s failed. err=%d", DEV_FILE, fd);
		exit(1);
	}

	memset(&data, 0, sizeof(data));
	data.a = 10;
	data.b = 20;
	strcpy(data.buff, "test");

	ioctl(fd, IOCTL_FOO_SET_A, &data);

	memset(&data, 0, sizeof(data));

	ioctl(fd, IOCTL_FOO_GET_A, &data);

	printf("a=%d, b=%d, buff=%s\n", data.a, data.b, data.buff);

	close(fd);
}

