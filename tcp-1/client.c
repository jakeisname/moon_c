
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

static int connect_server(peer_t *client, char *server_ip, int server_port)
{
	struct sockaddr_in server_addr;

	// create socket
	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket < 0) {
		perror("socket()");
		return -1;
	}

	// set up addres
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	server_addr.sin_port = htons(server_port);

	client->addres = server_addr;

	if (connect(client->socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) != 0) {
		perror("connect()");
		return -1;
	}

	printf("Connected to %s:%d.\n", server_ip, server_port);

	return 0;
}

static int build_fd_sets_for_client(peer_t *client, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
	/* read_fds */
	FD_ZERO(read_fds);
	FD_SET(client->socket, read_fds);

	/* write_fds, always something to send */
	FD_ZERO(write_fds);
	if (client->socket != NO_SOCKET && client->fifo.current > 0)
		FD_SET(client->socket, write_fds);

	/* except_fds */
	FD_ZERO(except_fds);
	FD_SET(client->socket, except_fds);

	return 0;
}

static int do_client_ex(peer_t *client, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds, int max_fds)
{
	int rc = 0;
	int i;

	/* All set fds should be checked. */
	if (FD_ISSET(client->socket, read_fds)) {
		rc = receive_from_peer(client, &handle_client_message);
		if (rc)
			return rc;
	}

	if (FD_ISSET(client->socket, write_fds)) {
		rc = send_to_peer(client);
		if (rc)
			return rc;
	}

	if (FD_ISSET(client->socket, except_fds)) {
		printf("except_fds for server.\n");
		shutdown_properly(EXIT_FAILURE);
	}

	return rc;
}

static int send_random_data(peer_t *client)
{
	int rc;
	long rnd = random();
	char buff[MAX_TEXT_SIZE];
	static int cnt = 1;

	switch (rnd % 3) {
		case 1 : 
			rc = send_data1(client, 11, cnt);
			break;

		case 2 : rc = send_data2(client, 22, cnt, 0);
			 break;

		default :
			 sprintf(buff, "test-%d", cnt);
			 rc = send_data3(client, buff);
	}

	if (!rc) {
		/* enqueue success */
		set_qna_state(client, 1);
		cnt++;
	}

	return rc;
}

int do_client(peer_t *client, char *server_ip, int server_port)
{
	fd_set read_fds;
	fd_set write_fds;
	fd_set except_fds;
	int i;
	int max_fds;
	int rc;
	
	while (1) {

		/* connect to server */
		printf("Try to connect to server=%s, port=%d)\n", server_ip, server_port);
		if (connect_server(client, server_ip, server_port) != 0) {
			sleep(2);
			continue;
		}

		while (1) {
			/* send data */
			if (!get_qna_state(client))
				send_random_data(client);

			build_fd_sets_for_client(client, &read_fds, &write_fds, &except_fds);
			max_fds = client->socket;

			rc = select(max_fds + 1, &read_fds, &write_fds, &except_fds, NULL);
			if (rc < 0) {
				if ((errno == EINTR) || (errno == EAGAIN))
					continue;

				printf("select() failed. err=%d(%s)\n",
						errno, strerror(errno));
				disconnect_peer(client);
				break;
			} else if (rc == 0) {
				/* you should never get here (bcoz no timeout) */
				printf("select() returns 0.\n");
				shutdown_properly(EXIT_FAILURE);
			}

			rc = do_client_ex(client, &read_fds, &write_fds, &except_fds, max_fds);
			if (rc < 0) {
				disconnect_peer(client);
				break;
			}

		}

		sleep(2);
	}

	return 0;
}
