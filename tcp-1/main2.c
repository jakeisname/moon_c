
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

peer_t client;
peer_t *g_peer = &client;

int main(int argc, char **argv)
{
	int i;
	char *server_ip;
	int port;	

	if (argc < 3) {
		printf("usage) %s <ip> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	server_ip = argv[1];
	port = atoi(argv[2]);

	/* for SIGINT & SIGPIPE */
	if (setup_signals() != 0)
		exit(EXIT_FAILURE);

	/* prepare client */
	client.socket = NO_SOCKET;
	create_peer(&client);

	/*initial count */
	client.trans_id = 1000;	

	/* do socket processing */
	do_client(&client, server_ip, port);
}



