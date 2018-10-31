#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/time.h>
#include "slist.c"
#include <sys/un.h>
#include <errno.h>

#define MAXLEN 40960

char* toUpper(char*);

#define SOCK_FILE "/tmp/test_socket"

int main(int argc, char **argv)
{
	slist_t *list = (slist_t*)malloc(sizeof(slist_t));
	slist_init(list);
	fd_set read_fds, write_fds;				// temp file descriptors list for select()
	int sockfd;								// socket descriptor
#ifdef __UDP
	struct sockaddr_in srv, cli_addr;		// srv used by bind()
#else
	struct sockaddr_un srv, cli_addr;		// srv used by bind()
#endif
	socklen_t clilen = sizeof(cli_addr);
	int nbytes;
	int portno = 0;	

#ifdef __UDP
	//check command line arguments 
	if (argc != 2) {
		fprintf(stderr, "Usage: ./server %s <port>\n", argv[1]);
		exit(1);
	}
	portno = atoi(argv[1]);
#endif

#ifdef __UDP
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
#else
	if ((sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0)) < 0){
#endif
		perror("ERROR opening socket");
		exit(1);
	}
	else
		printf("Server : Socket() successful\n");
    
#ifdef __UDP
  /* bind: use the Internet address family: */
	/* create the socket */
	srv.sin_family = AF_INET;
	/* bind: socket ‘sockfd’ to port portno*/
	srv.sin_port = htons(portno);
	/* bind: a client may connect to any of my addresses */
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
	srv.sun_family = AF_LOCAL;
	strncpy(srv.sun_path, SOCK_FILE, sizeof(srv.sun_path));
	srv.sun_path[sizeof(srv.sun_path) - 1] = 0;
#endif
     
	if(bind(sockfd, (struct sockaddr*) &srv, sizeof(srv)) < 0){
		perror("ERROR on binding");
		close(sockfd);
		exit(1);
	}
	else
		printf("Server : bind() successful\n");
	//Now the UDP server is ready to accept packets…
    while(1){ 			
		char* buf = (char*)malloc(MAXLEN);				// message buf
		char* copybuf = (char*)malloc(MAXLEN);
		int j;
		memset(buf, 0, strlen(buf));
		bzero(buf, MAXLEN);
		FD_ZERO(&read_fds);
		FD_SET(sockfd, &read_fds);
		
		// nbytes = recvfrom(sockfd, buf, MAXLEN, 0, (struct sockaddr*)&cli_addr, &clilen);
		// printf("n=%d\n", nbytes);

		int n = select(sockfd+1, &read_fds, &write_fds, 0, 0);
		if(n < 0)
		{
			perror("ERROR Server : select()\n");
			close(sockfd);
			exit(1);
		}
		if(FD_ISSET(sockfd, &read_fds)) {
			printf("Server is ready to read\n");
			nbytes = recvfrom(sockfd, buf, MAXLEN, 0, (struct sockaddr*)&cli_addr, &clilen);
			printf("received=%d\n", nbytes);
			if (nbytes < 0){
				perror("ERROR in recvfrom()");
				close(sockfd);
				exit(1);
			}
#ifdef __UDP
			j = ntohs(cli_addr.sin_port);
#else
			j = 0;
#endif
			FD_CLR(sockfd, &read_fds);
		}
		if(strlen(buf) != 0){		
			memcpy(copybuf, buf, strlen(buf));


#if 0

#ifdef __UDP
			int i = strlen(buf) + 3 + 
				strlen(inet_ntoa(cli_addr.sin_addr)) + 
				ntohs(cli_addr.sin_port);
#else
			int i = strlen(buf) + 20;
#endif
			char* c = (char*) malloc(i);
			char ch[5];
#ifdef __UDP
			strcpy(c, inet_ntoa(cli_addr.sin_addr));
#else
			strcat(c, "localhost");
#endif
			strcat(c, " : ");
			sprintf(ch, "%d", j);
			strcat(c, ch);
			strcat(c, " : ");
			strcat(c, buf);
			slist_append(list, c);
			//printlist(list);				//you may see the list
#endif
			if(list == NULL){
				perror("List is empty. Server is not ready to write.");
				close(sockfd);
				exit(1);
			}
			else{
				FD_ZERO(&write_fds);
				FD_SET(sockfd, &write_fds);
				if(FD_ISSET(sockfd, &write_fds)){
					toUpper(copybuf);
#ifdef __UDP
					nbytes = sendto(sockfd, copybuf, MAXLEN, 0, 
					    (struct sockaddr*) &cli_addr, sizeof(cli_addr));
#else
					nbytes = sendto(sockfd, copybuf, MAXLEN, 0, 
					    (struct sockaddr*) &cli_addr, sizeof(cli_addr));
#endif
					if(nbytes < 0) {
						printf("ERROR in sendto. err=%d(%s)",
							errno, strerror(errno));
						close(sockfd);
						exit(1);
					}
					printf("Server is ready to write\n");
					//printf("Send to client: %s", copybuf);
					//printf("==========================\n");
					printf("\n");
					memset(&copybuf, 0, sizeof(copybuf));
					FD_CLR(sockfd, &write_fds);
				}
			}

//			bzero(ch, strlen(ch));
		}
		memset(&buf, 0, sizeof(buf));
		free(buf);
		free(copybuf);
	}
	free(list);
	close(sockfd);
	return 0;
}
char* toUpper(char* str){
	if(str != NULL){	
		int i;
		for(i = 0; i<strlen(str); i++)
		{
			if( str[i] <= 'z' && str[i] >= 'a')
			{
				str[i] += 'A' - 'a';
			}
		}
		return str;
	}
	return NULL;
}
