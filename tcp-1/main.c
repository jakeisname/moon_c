
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
peer_t *g_peer = &server;

static void show_usage(char *name)
{
	printf("usage) %s [-p <port>] [-d]\n", name);
}


int main(int argc, char **argv)
{
	int i;
	int port = DEFAULT_PORT;	/* server listen port */
	const char *optstring = "p:d"; 
	char option; 

	optind = 1; 
	while (-1 != (option = getopt(argc, argv, optstring))) { 
		switch (option) { 
			case 'p' : 
				port = atoi(optarg); 
				break; 
			case 'd' : 
				enable_dump();
				break; 
			default: 
				show_usage((char *) argv[0]);
				exit(EXIT_FAILURE);
		}; 
	};

	/* for SIGINT & SIGPIPE */
	if (setup_signals() != 0)
		exit(EXIT_FAILURE);

	/* bind & listen server socket */
	if (start_listen_socket(port, &server.socket) != 0)
		exit(EXIT_FAILURE);

	/* Set nonblock for stdin. */
	set_sock_nonblocking(STDIN_FILENO);

	/* prepare clients */
	for (i = 0; i < MAX_CLIENTS; ++i) {
		clients[i].socket = NO_SOCKET;
		create_peer(&clients[i]);
	}

	/* do socket processing */
	do_server();
}



