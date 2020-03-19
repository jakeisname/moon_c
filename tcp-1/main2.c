
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

static void show_usage(char *name)
{
	printf("usage) %s [-c <ip>] [-p <port>] [-t]\n", name);
}
	
int main(int argc, char **argv)
{
	int i;
	char *server_ip = SERVER_IPV4_ADDR;
	int port = DEFAULT_PORT;	

	const char *optstring = "c:p:dt"; 
	char option; 

	optind = 1; 
	while (-1 != (option = getopt(argc, argv, optstring))) { 
		switch (option) { 
			case 'c' : 
				server_ip = optarg;
				break; 
			case 'p' : 
				port = atoi(optarg); 
				break; 
			case 'd' : 
				enable_dump();
				break; 
			case 't' : 
				enable_trouble();
				break; 
			default: 
				show_usage((char *) argv[0]);
				exit(EXIT_FAILURE);
		}; 
	};

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



