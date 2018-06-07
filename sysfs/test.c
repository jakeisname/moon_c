#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <poll.h>

#define FOO_VALUE "/sys/foo/foo_value"
#define FOO_NOTIFY "/sys/foo/foo_notify"

int main(int argc, char **argv)
{
	int len, value_fd, notify_fd, recv;
	char attr_data[100];
	struct pollfd fds[1];

	if ((value_fd = open(FOO_VALUE, O_RDWR)) < 0)
	{
		printf("Unable to open %s\n", FOO_VALUE);
		exit(1);
	}

	if ((notify_fd = open(FOO_NOTIFY, O_RDWR)) < 0)
	{
		printf("Unable to open %s\n", FOO_NOTIFY);
		exit(1);
	}

	fds[0].fd = notify_fd;
	fds[0].events = POLLPRI | POLLERR;

	/* read garvage data once */
	len = read(notify_fd, attr_data, 100);

	fds[0].revents = 0;

	/* wait until 60 sec */
	recv = poll(fds, 1, 60000);
	if (recv < 0)
	{
		perror("poll error");
	}
	else if (recv == 0)
	{
		printf("Timeout!\n");
	}
	else
	{
		memset(attr_data, 0, 100);
		len = lseek(value_fd, 0, SEEK_SET);
		len = read(value_fd, attr_data, 100);
		printf( "foo_value=%s (len=%d)\n", attr_data, len);
	}

	close(notify_fd);
	close(value_fd);
}
