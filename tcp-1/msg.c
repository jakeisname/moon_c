
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"

int prepare_message(char *sender, char *data, message_t *message)
{
	sprintf(message->sender, "%s", sender);
	sprintf(message->data, "%s", data);
	return 0;
}

int print_message(message_t *message)
{
	printf("RX: %s", message->data);
	return 0;
}

int handle_received_message(peer_t *peer)
{
	message_t *rx_msg = &peer->rx_buff;
	message_t *tx_msg = &peer->tx_buff;

	print_message(rx_msg);

	if (!strncmp(rx_msg->data, "quit", 4))
		return -1;
	if (!strncmp(rx_msg->data, "exit", 4))
		return -1;

	prepare_message(SERVER_NAME, &rx_msg->data[0], tx_msg);
	printf("TX: %s", tx_msg->data);
	peer_add_to_send(peer, tx_msg);
	return 0;
}
