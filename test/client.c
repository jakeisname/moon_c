#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <errno.h>

#define MAXLINE   40960
#define SERV_PORT 8686

#define SOCK_FILE "/tmp/test_socket"

int main(int argc, char** argv)
{
    /* standard variables used in a udp client */
    int                     sockfd;
    int                     recvlen;
#ifdef __UDP
    struct sockaddr_in      servAddr;
#else
    struct sockaddr_un      servAddr;
#endif
    const struct sockaddr*  servAddr_in;
    socklen_t               servLen;
    char                    sendLine[MAXLINE];
    char                    recvLine[MAXLINE + 1];
    int			    port = 0;

#ifdef __UDP
    if (argc != 3) {
        printf("usage: udpcli <IP address> <port>\n");
        return 1;
    }

    port = atoi(argv[2]);
#else
#endif

#ifdef __UDP
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
#else
    if ( (sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0) {
#endif
       printf("cannot create a socket.");
       return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));

#ifdef __UDP
    servAddr.sin_family = PF_INET;
    servAddr.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &servAddr.sin_addr);
#else
    servAddr.sun_family = PF_LOCAL;
    strncpy(servAddr.sun_path, SOCK_FILE, sizeof(servAddr.sun_path));
    servAddr.sun_path[sizeof(servAddr.sun_path) - 1] = 0;

#if 0
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
            printf("connect() failed. errno=%d(%s)\n", errno, strerror(errno));
	    return 1;
    }
#endif
#endif

/****************************************************************************/
/*               Code for sending the datagram to the server                */
    servAddr_in = (struct sockaddr*) &servAddr;
    servLen = sizeof(servAddr);

    /* Loop while user is giving input or until EOF is read */
    while (fgets(sendLine, MAXLINE, stdin) != NULL) {

    /* Attempt to send sendLine to the server */
        if ( ( sendto(sockfd, sendLine, strlen(sendLine) - 1, 0, servAddr_in,
                        servLen)) == -1) {
            printf("Error in sending.\n");
	    continue;
        }

    /* Attempt to receive recvLine from the server */
        if ( (recvlen = recvfrom(sockfd, recvLine, MAXLINE, 0, NULL, NULL))
                == -1) {
            printf("Error in receiving.\n");
        }

        recvLine[recvlen] = '\0';
        fputs(recvLine, stdout);
    }
/*                                                                          */
/****************************************************************************/

    return 0;
}

