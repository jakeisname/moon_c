
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

peer_t server; 			/* used for server */
peer_t clients[MAX_CLIENTS];	/* used for clients */

int main(int argc, char **argv)
{
	int i;
	int flag;
	int port;	/* server listen port */

	if (argc < 2) {
		printf("usage) %s <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	port = atoi(argv[1]);

	/* for SIGINT & SIGPIPE */
	if (setup_signals() != 0)
		exit(EXIT_FAILURE);

	if (start_listen_socket(port, &server.socket) != 0)
		exit(EXIT_FAILURE);

	/* Set nonblock for stdin. */
	flag = fcntl(STDIN_FILENO, F_GETFL, 0);
	flag |= O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flag);

	for (i = 0; i < MAX_CLIENTS; ++i) {
		clients[i].socket = NO_SOCKET;
		create_peer(&clients[i]);
	}

	do_server();
}



