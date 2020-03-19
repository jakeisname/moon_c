
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "common.h"


int set_reuseaddr_opt(int fd)
{
	int reuse = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
		printf("setsockopt(SO_REUSE_ADDR) failed. errno=%d(%s)\n", 
				errno, strerror(errno));
		return -1;
	}
	return 0;
}

int set_sock_nonblocking(int fd)
{
	int flag;

	flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);

	return 0;
}


int set_sock_timeout(int fd, int sec)
{
	struct timeval  tv = { .tv_sec = sec, .tv_usec = 0 };

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, 
				(char *) &tv, sizeof(struct timeval)) != 0) {
		printf("setsockopt(SO_REUSE_ADDR) failed. errno=%d(%s)\n", 
				errno, strerror(errno));
		return -1;
	}

	return 0;
}

/* 
 * If there is no traffic by the @idle period, 
 * a keepalive packet is transmitted at a specified @interval by @cnt.
 * 
 * fd: file handle (socket number)
 * idle: (seconds)
 * interval: (seconds)
 * cnt:
 */
void set_sock_keepallive(int fd, int idle, int interval, int cnt)
{
    int yes = 1;

    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(int));
}

void set_sock_nodelay(int fd)
{
    int yes = 1;

    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int));
}

