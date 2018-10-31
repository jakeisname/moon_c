#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/time.h>
#include <sys/un.h>
#include <errno.h>
#include <stddef.h>

#define MAXLEN 40960

#define __A1

char *toUpper(char *);

#define SOCK_FILE "/tmp/test_socket"

static int read_from_client(int filedes);

int main(int argc, char **argv)
{
	fd_set read_fds, write_fds;	// temp file descriptors list for select()
	fd_set active_fds;
	int sockfd;			// socket descriptor
	struct sockaddr_un srv, cli_addr;
	int nbytes;
	int size;
	int ret;
	struct sockaddr_in clientname;
	int n;
	int i;
	int len;
	int peerfd;

	unlink(SOCK_FILE);

#ifdef __A1
	if ((sockfd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0)
#else
	if ((sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0)) < 0)
#endif
	{
		perror("ERROR opening socket");
		exit(1);
	}
	
	printf("Server : Socket() successed. sock=%d\n", sockfd);

	srv.sun_family = AF_LOCAL;
	strncpy(srv.sun_path, SOCK_FILE, sizeof(srv.sun_path));
	size = offsetof(struct sockaddr_un, sun_path) + strlen(srv.sun_path) + 1;

	if (bind(sockfd, (struct sockaddr *) &srv, size) < 0){
		perror("ERROR on binding");
		close(sockfd);
		exit(1);
	} else
		printf("Server : bind() successful\n");

	printf("try to listen()\n");
	if (listen(sockfd, 10) < 0) {
		perror("listen error");
		exit(-1);
	}
	printf("listen() successed.\n");

	FD_ZERO(&active_fds);
	FD_SET(sockfd, &active_fds);

	while (1) {
		read_fds = active_fds;

		printf("try to master select()\n");
		n = select(sockfd+1, &read_fds, 0, 0, 0);
		if (n < 0) {
			perror("ERROR Server : select()\n");
			close(sockfd);
			exit(1);
		}
		printf("select() successed. n=%d. FDS=%d\n", n, FD_SETSIZE);

		for (i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &read_fds)) {
				/* new client */
				if (i == sockfd) {

					printf("try to accept(). i=%d\n", i);
					len = sizeof(clientname);
					n = accept(sockfd, (struct sockaddr *) &clientname, &size);
					if (n < 0) {
						perror("accept error");
						break;
					}

					printf("accept() successed. peer=%d\n", n);

					FD_SET(n, &active_fds);
					peerfd = n;
				} else {
					n = read_from_client(i);
					if (n < 0) {
						/* remove client when lost connection*/
						close(i);
						FD_CLR(i, &active_fds);
						peerfd = -1;
					}
				}
			}
		}
	}

	if (sockfd != -1)
		close(sockfd);

	if (peerfd != -1)
		close(peerfd);

	return 0;
}

char *toUpper(char *str) 
{
	if (str != NULL) {	
		int i;
		for (i = 0; i < strlen(str); i++) {
			if( str[i] <= 'z' && str[i] >= 'a')
				str[i] += 'A' - 'a';
		}
		return str;
	}

	return NULL;
}

static int read_from_client(int filedes)
{
	char buffer[MAXLEN];
	int nbytes;
	int seq = 0;
	int rx_seq = 0;
	int oops = 0;

	printf("%s(%d): try to read() new_sock=%d\n", __func__, __LINE__, filedes);

	nbytes = read(filedes, buffer, MAXLEN);
	if (nbytes < 0) {
		/* Read error. */
		perror ("read");
		exit (EXIT_FAILURE);
	} else if (nbytes == 0)
		/* End-of-file. */
		return -1;

	/* Data read. */
	fprintf (stderr, "Server: got message: new_sock=%d, `%s'\n", filedes, buffer);
	buffer[nbytes] = 0;
	printf("%s: received=%d, oops=%d\n", buffer, nbytes, oops);

	seq = atoi(&buffer[5]);
	if (seq != rx_seq++) {
		rx_seq = seq + 1;
		oops++;
	}

	return 0;
}

#if 0
int write_to_client()
{
	if (strlen(buf) == 0) 
		return 0;

	memcpy(copybuf, buf, strlen(buf));

	FD_ZERO(&write_fds);
	FD_SET(sockfd, &write_fds);

	if (FD_ISSET(sockfd, &write_fds)) {
		toUpper(copybuf);
#ifdef __XX
		nbytes = sendto(sockfd, copybuf, MAXLEN, 0, 
				(struct sockaddr *) &cli_addr, sizeof(cli_addr));
#else
		//				nbytes = sendto(sockfd, copybuf, 1, 0, 
		// (struct sockaddr*) &cli_addr, sizeof(cli_addr));
		nbytes = 1;
#endif
		if (nbytes < 0) {
			printf("ERROR in sendto. err=%d(%s)",
					errno, strerror(errno));
			close(sockfd);
			exit(1);
		}

		//printf("Send to client: %s", copybuf);
		//printf("==========================\n");
		memset(&copybuf, 0, sizeof(copybuf));
		FD_CLR(sockfd, &write_fds);
	}

	memset(&buf, 0, sizeof(buf));
	free(buf);
	free(copybuf);
}
#endif
