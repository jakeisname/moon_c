#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <errno.h>

#define MAXLINE   40960
#define SERV_PORT 8686

#define SOCK_FILE "/tmp/test_socket"

#define __A1

int main(int argc, char** argv)
{
	/* standard variables used in a udp client */
	int                     sockfd;
	int                     recvlen;
	struct sockaddr_un      servAddr;
	const struct sockaddr*  servAddr_in;
	socklen_t               servLen;
	char                    sendLine[MAXLINE];
	char                    recvLine[MAXLINE + 1];
	int			    port = 0;
	int cnt;
	int n, len;

#ifdef __A1
	if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
#else
	if ((sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
#endif
	{
		printf("cannot create a socket.");
		return 1;
	}

	memset(&servAddr, 0, sizeof(servAddr));

	servAddr.sun_family = PF_LOCAL;
	strncpy(servAddr.sun_path, SOCK_FILE, sizeof(servAddr.sun_path) - 1);

	printf("created socket.\n");
	if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) == -1) {
		printf("connect() failed. errno=%d(%s)\n", errno, strerror(errno));
		return 1;
	}

	printf("connect.\n");
	/****************************************************************************/
	/*               Code for sending the datagram to the server                */
	servAddr_in = (struct sockaddr*) &servAddr;
	servLen = sizeof(servAddr);

	/* Loop while user is giving input or until EOF is read */
	while (1) {
#if 0
		if (fgets(sendLine, MAXLINE, stdin) == NULL)
			return 0;
#else
		sprintf(sendLine, "test-%8d\n", cnt++);
#endif

		/* Attempt to send sendLine to the server */
		len = strlen(sendLine);
#ifdef __A1
		printf("try to write()\n");
		// n = write(sockfd, sendLine, len);
		n = send(sockfd, sendLine, len, 0);
		printf("write() n=%d\n", n);
#else
		if ((sendto(sockfd, sendLine, strlen(sendLine), 0, servAddr_in,
						servLen)) == -1) 
#endif
		if (n != len)
		{
			printf("Error in sending. n=%d, err=%d(%s)\n", 
				n, errno, strerror(errno));
			continue;
		}

#if 0
		/* Attempt to receive recvLine from the server */
		if ( (recvlen = recvfrom(sockfd, recvLine, MAXLINE, 0, NULL, NULL))
				== -1) {
			printf("Error in receiving.\n");
		}

		recvLine[recvlen] = '\0';
		fputs(recvLine, stdout);
#endif
	}
	/*                                                                          */
	/****************************************************************************/

	return 0;
}

#if 0

bytesReceived = 0;
while (bytesReceived < BUFFER_LENGTH)
{
	rc = recv(sd, & buffer[bytesReceived],
			BUFFER_LENGTH - bytesReceived, 0);
	if (rc < 0)
	{
		perror("recv() failed");
		break;
	}
	else if (rc == 0)
	{
		printf("The server closed the connection\n");
		break;
	}

	/*****************************************************************/
	/* Increment the number of bytes that have been received so far  */
	/*****************************************************************/
	bytesReceived += rc;
}

#endif
