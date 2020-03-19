
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


/* Start listening from listen socket */
int start_listen_socket(int port, int *listen_sock)
{
	struct sockaddr_in my_addr;

	/* Obtain a file descriptor for our "listening" socket. */
	*listen_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (*listen_sock < 0) {
		printf("socket() failed. errno=%d(%s)\n",
				errno, strerror(errno));
		return -1;
	}

	/* socket option: SO_REUSEADDR */
	if (set_reuseaddr_opt(*listen_sock) < 0)
		return -1;

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4_ADDR);
	my_addr.sin_port = htons(port);

	if (bind(*listen_sock, (struct sockaddr*)&my_addr, 
				sizeof(struct sockaddr)) != 0) {
		printf("bind() failed. errno=%d(%s)\n",
				errno, strerror(errno));
		return -1;
	}

	/* start accept client connections */
	if (listen(*listen_sock, 10) != 0) {
		printf("listen() failed. errno=%d(%s)\n",
				errno, strerror(errno));
		return -1;
	}
	printf("Accepting connections on port %d.\n", port);

	return 0;
}

static int add_to_new_connection(int new_client_sock, struct sockaddr_in *client_addr)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; ++i) {
		/* success for client */
		if (clients[i].socket == NO_SOCKET) {
			clients[i].socket = new_client_sock;
			clients[i].addres = *client_addr;
			clients[i].tx_bytes = -1;
			clients[i].rx_bytes = 0;
			clients[i].seq_id = 0;
			return 0;
		}
	}

	/* MAX connection limit */
	return -1;
}

static int handle_new_connection()
{
	int i;
	int rc;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int new_client_sock;
	char client_ipv4_str[INET_ADDRSTRLEN];

	memset(&client_addr, 0, client_len);
	new_client_sock = accept(server.socket, 
			(struct sockaddr *) &client_addr, &client_len);
	if (new_client_sock < 0) {
		printf("accept() failed. errno=%d(%s)\n",
				errno, strerror(errno));
		return -1;
	}
	
	/* no traffic during 10 seconds then 
	 *	send 3 keepalive packets every 5 seconds */
	set_sock_keepallive(new_client_sock, 10, 5, 3);

	inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);
	printf("Incoming connection from %s(%d).\n", 
			client_ipv4_str, ntohs(client_addr.sin_port));

	rc = add_to_new_connection(new_client_sock, &client_addr);
	if (rc < 0) {
		printf("There is too much connections. Close new connection %s(%d).\n", 
				client_ipv4_str, ntohs(client_addr.sin_port));
		close(new_client_sock);	
	}

	return rc;

}

static int close_client_connection(peer_t *client)
{
	printf("Close client socket for %s.\n", peer_get_addres_str(client));
	printf("\n\n\n");

	close(client->socket);
	client->socket = NO_SOCKET;
	dequeue_all(&client->fifo);
	client->tx_bytes = -1;
	client->rx_bytes = 0;
	client->seq_id = 0;
}

int get_client_name(int argc, char **argv, char *client_name)
{
	if (argc > 1)
		strcpy(client_name, argv[1]);
	else
		strcpy(client_name, "no name");

	return 0;
}


static int build_fd_sets(fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
	int i;

	/* read_fds */
	FD_ZERO(read_fds);
	FD_SET(STDIN_FILENO, read_fds);		/* add stdin */
	FD_SET(server.socket, read_fds);	/* add listen socket */

	for (i = 0; i < MAX_CLIENTS; ++i)	/* add clients */
		if (clients[i].socket != NO_SOCKET)
			FD_SET(clients[i].socket, read_fds);

	/* write_fds */
	FD_ZERO(write_fds);
	for (i = 0; i < MAX_CLIENTS; ++i)	/* add clients */
		if (clients[i].socket != NO_SOCKET && clients[i].fifo.current > 0) {
			FD_SET(clients[i].socket, write_fds);
		}

	/* escept_fds */
	FD_ZERO(except_fds);
	FD_SET(STDIN_FILENO, except_fds);	/* add stdin */
	FD_SET(server.socket, except_fds);	/* add listen socket */
	for (i = 0; i < MAX_CLIENTS; ++i)	/* add clients */
		if (clients[i].socket != NO_SOCKET)
			FD_SET(clients[i].socket, except_fds);

	return 0;
}  



static int get_max_fds()
{
	int max_fds = server.socket;
	int i;

	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i].socket > max_fds)
			max_fds = clients[i].socket;
	}
	return max_fds;
}


static int do_server_ex(fd_set *read_fds, fd_set *write_fds, fd_set *except_fds, int max_fds)
{
	int rc;
	int i;

	/* All set fds should be checked. */
	if (FD_ISSET(STDIN_FILENO, read_fds)) {
		if (handle_read_from_stdin() != 0)
			shutdown_properly(EXIT_FAILURE);
	}

	if (FD_ISSET(server.socket, read_fds)) {
		handle_new_connection();
	}

	if (FD_ISSET(STDIN_FILENO, except_fds)) {
		printf("except_fds for stdin.\n");
		shutdown_properly(EXIT_FAILURE);
	}

	if (FD_ISSET(server.socket, except_fds)) {
		printf("except_fds for server.\n");
		shutdown_properly(EXIT_FAILURE);
	}

	/* check all client sockets */
	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i].socket != NO_SOCKET && 
				FD_ISSET(clients[i].socket, read_fds)) {
			rc = receive_from_peer(&clients[i], &handle_server_message);
			if (rc < 0)
				close_client_connection(&clients[i]);
			
		}

		if (clients[i].socket != NO_SOCKET && 
				FD_ISSET(clients[i].socket, write_fds)) {
			rc = send_to_peer(&clients[i]);
			if (rc < 0)
				close_client_connection(&clients[i]);
		}

		if (clients[i].socket != NO_SOCKET && 
				FD_ISSET(clients[i].socket, except_fds)) {
			printf("except_fds for client.\n");
			close_client_connection(&clients[i]);
		}
	}
}

int do_server()
{
	fd_set read_fds;
	fd_set write_fds;
	fd_set except_fds;
	int i;
	int max_fds;
	int rc;

	printf("Waiting for incoming connections.\n");
	while (1) {
		build_fd_sets(&read_fds, &write_fds, &except_fds);
		max_fds = get_max_fds();

		rc = select(max_fds + 1, &read_fds, &write_fds, &except_fds, NULL);
		switch (rc) {
			case -1:
				if ((errno == EINTR) || (errno == EAGAIN))
					continue;

				printf("select() failed. err=%d(%s)\n",
						errno, strerror(errno));
				shutdown_properly(EXIT_FAILURE);

			case 0:
				/* you should never get here (bcoz no timeout) */
				printf("select() returns 0.\n");
				shutdown_properly(EXIT_FAILURE);

			default:
				rc = do_server_ex(&read_fds, &write_fds, &except_fds, max_fds);
		}
	}

	return 0;
}

