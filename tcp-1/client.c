
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"



int connect_server(peer_t *server)
{
	// create socket
	server->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server->socket < 0) {
		perror("socket()");
		return -1;
	}

	// set up addres
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4_ADDR);
	server_addr.sin_port = htons(SERVER_LISTEN_PORT);

	server->addres = server_addr;

	if (connect(server->socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) != 0) {
		perror("connect()");
		return -1;
	}

	printf("Connected to %s:%d.\n", SERVER_IPV4_ADDR, SERVER_LISTEN_PORT);

	return 0;
}



int build_fd_sets_for_client(peer_t *server, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
	/* read_fds */
	FD_ZERO(read_fds);
	FD_SET(STDIN_FILENO, read_fds);
	FD_SET(server->socket, read_fds);

	/* write_fds, there is something to send, set up write_fd for server socket */
	FD_ZERO(write_fds);
	if (server->send_buffer.current > 0)
		FD_SET(server->socket, write_fds);

	/* except_fds */
	FD_ZERO(except_fds);
	FD_SET(STDIN_FILENO, except_fds);
	FD_SET(server->socket, except_fds);

	return 0;
}

