
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

/**************************************************************
 *
 * for server
 *
 **************************************************************/

typedef struct  {
 	void (*convert_ntoh_func) (peer_t *peer);
	int (*process_func) (peer_t *peer);
} server_msg_handler_t;


static void convert_ntoh_data1(peer_t *peer)
{
	data1_t *d = (data1_t *) &peer->rx_buff.data[0];

	d->a = ntohl(d->a);
	d->b = ntohl(d->b);
}

static void convert_ntoh_data2(peer_t *peer)
{
	data2_t *d = (data2_t *) &peer->rx_buff.data[0];

	d->a = ntohl(d->a);
	d->b = ntohl(d->b);
	d->c = ntohl(d->c);
}

static int process_data1(peer_t *peer)
{
	data1_t *d = (data1_t *) &peer->rx_buff.data[0];

	printf("a=%d, b=%d\n", d->a, d->b);

	usleep(100000);

	return 0;
}

static int process_data2(peer_t *peer)
{
	data2_t *d = (data2_t *) &peer->rx_buff.data[0];
	data2_t *tx_d = (data2_t *) &peer->tx_buff.data[0];

	tx_d->c = d->a + d->b;

	printf("a=%d, b=%d, c=%d->%d\n", d->a, d->b, d->c, tx_d->c);

	return 0;
}

static int process_data3(peer_t *peer)
{
	data3_t *d = (data3_t *) &peer->rx_buff.data[0];

	printf("text=%s\n", d->text);

	return 0;
}

static server_msg_handler_t server_msg_handler[MSG_TYPE_MAX] = {
	{ NULL,			NULL },
	{ convert_ntoh_data1, 	process_data1 },
	{ convert_ntoh_data2, 	process_data2 },
	{ NULL, 		process_data3 },
};

static void convert_ntoh_message_common(peer_t *peer)
{
	/* convert from rx_buff */
	message_t *msg = &peer->rx_buff;

	msg->magic = 	ntohl(msg->magic);
	msg->seq_id = 	ntohl(msg->seq_id);
	msg->trans_id = ntohl(msg->trans_id); 
	msg->msg_type = ntohl(msg->msg_type);
	msg->msg_len = 	ntohl(msg->msg_len);
}

static void convert_ntoh_message_each_type(peer_t *peer)
{
	message_t *msg = &peer->rx_buff;
	msg_type_t type = msg->msg_type;

	if (type <= MSG_TYPE_UNKNOWN || type >= MSG_TYPE_MAX)
		return;

	if (server_msg_handler[type].convert_ntoh_func)
		server_msg_handler[type].convert_ntoh_func(peer);
}

static void print_message_common(peer_t *peer)
{
	message_t *msg = &peer->rx_buff;

	printf("RX: seq_id=%d, trans_id=%d, type=%d, len=%d: ", 
		msg->seq_id, msg->trans_id, msg->msg_type, msg->msg_len);
}

static int process_message_each_type(peer_t *peer)
{
	message_t *msg = &peer->rx_buff;
	msg_type_t type = msg->msg_type;

	if (type <= MSG_TYPE_UNKNOWN || type >= MSG_TYPE_MAX)
		return -1;

	if (server_msg_handler[type].process_func)
		return server_msg_handler[type].process_func(peer);

	return 0;
}

static int is_server_invalid_packet(peer_t *peer)
{
	message_t *msg = &peer->rx_buff;
	message_t *tx_msg = &peer->tx_buff;

	if (msg->magic != MAGIC_NUM) {
		if (msg->magic == htonl(MAGIC_NUM))
			printf("peer endian mismatched. this=0x%x, peer=0x%x\n",
					MAGIC_NUM, msg->magic);
		else
			printf("magic number is not matched. this=0x%x, peer=0x%x\n",
					MAGIC_NUM, msg->magic);
		return -1;
	}

	if (peer->seq_id != msg->seq_id) {
		printf("seq_id is not matched. this=%d, peer=%d\n",
				peer->seq_id, msg->seq_id);
		return -1;
	}

	return 0;
}


/* RX processing for SERVER */
int handle_server_message(peer_t *peer)
{
	message_t *rx_msg = &peer->rx_buff;
	message_t *tx_msg = &peer->tx_buff;

	convert_ntoh_message_common(peer);
	convert_ntoh_message_each_type(peer);

	if (is_server_invalid_packet(peer)) {
		dump_packet("RX-(invalid)----> ", 
			(char *) &peer->rx_buff, 0, peer->rx_bytes);
		return -1;
	}

	/* send back to client as same data */
	memcpy(tx_msg, rx_msg, sizeof(message_t));
	tx_msg->seq_id = peer->seq_id;

	print_message_common(peer);

	process_message_each_type(peer);

	peer_add_to_send(peer);

	inc_seq_id(peer);

	return 0;
}



/**************************************************************
 *
 * for client
 *
 **************************************************************/

typedef struct  {
	void (*convert_hton_func) (peer_t *peer);
 	int (*process_func) (peer_t *peer);
} client_msg_handler_t;

static void convert_hton_data1(peer_t *peer)
{
	data1_t *d = (data1_t *) &peer->tx_buff.data[0];

	d->a = htonl(d->a);
	d->b = htonl(d->b);
}

static void convert_hton_data2(peer_t *peer)
{
	data2_t *d = (data2_t *) &peer->tx_buff.data[0];

	d->a = htonl(d->a);
	d->b = htonl(d->b);
	d->c = htonl(d->c);
}

static int client_process_data1(peer_t *peer)
{
	data1_t *d = (data1_t *) &peer->rx_buff.data[0];

	printf("a=%d, b=%d\n", d->a, d->b);

	return 0;
}

static int client_process_data2(peer_t *peer)
{
	data2_t *d = (data2_t *) &peer->rx_buff.data[0];

	printf("a=%d, b=%d, c=%d\n", d->a, d->b, d->c);

	return 0;
}

static int client_process_data3(peer_t *peer)
{
	data3_t *d = (data3_t *) &peer->rx_buff.data[0];

	printf("text=%s\n", d->text);

	return 0;
}

static client_msg_handler_t client_msg_handler[MSG_TYPE_MAX] = {
	{ NULL },
	{ convert_hton_data1, 	client_process_data1 },
	{ convert_hton_data2,	client_process_data2 },
	{ NULL,			client_process_data3 },
};

//#define MAKE_BUG
static void prepare_common_data(peer_t *peer, message_t *msg, msg_type_t type, int msg_len)
{
	memset(msg, 0, sizeof(message_t));

	msg->magic = MAGIC_NUM;
	msg->seq_id = peer->seq_id;
	msg->trans_id = peer->trans_id;
	msg->msg_type = type;
	msg->msg_len = msg_len;

#ifdef MAKE_BUG
	static int cnt1 = 0;
	if ((random() % 100) == 0)
		msg->magic = htonl(MAGIC_NUM);
	if ((random() % 100) == 0)
		msg->magic = 999;
	if ((random() % 100) == 0)
		msg->seq_id = 888;
	if ((random() % 100) == 0)
		peer->trans_id++;
#endif
}

void convert_hton_common_data(peer_t *peer)
{
	message_t *msg = &peer->tx_buff;

	msg->magic = 	htonl(msg->magic);
	msg->seq_id = 	htonl(msg->seq_id);
	msg->trans_id = htonl(msg->trans_id); 
	msg->msg_type = htonl(msg->msg_type); 
	msg->msg_len = 	htonl(msg->msg_len); 
}

void convert_hton_each_data_type(peer_t *peer)
{
	/* convert from tx_buff */
	message_t *msg = &peer->tx_buff;
	msg_type_t type = msg->msg_type;

	if (type <= MSG_TYPE_UNKNOWN || type >= MSG_TYPE_MAX)
		return;

	if (client_msg_handler[type].convert_hton_func)
		client_msg_handler[type].convert_hton_func(peer);
}

static void process_client_message_each_type(peer_t *peer)
{
	message_t *msg = &peer->rx_buff;
	msg_type_t type = msg->msg_type;

	if (type <= MSG_TYPE_UNKNOWN || type >= MSG_TYPE_MAX) {
		printf("received unknown message type\n");
		return;
	}

	if (client_msg_handler[type].process_func)
		client_msg_handler[type].process_func(peer);

	printf("\n");
}

/* client send message to server */
static int client_send_msg(peer_t *peer, msg_type_t type, int data_len, void *data)
{
	message_t msg;
	int rc;

	/* add common data */
	prepare_common_data(peer, &msg, type, data_len);

	memcpy(&msg.data[0], data, data_len);

	/* do not convert data before enqueue */
	rc = enqueue(&peer->fifo, &msg);

	return rc;
}

static int is_client_invalid_packet(peer_t *peer)
{
	message_t *msg = &peer->rx_buff;
	message_t *tx_msg = &peer->tx_buff;

	if (msg->magic != MAGIC_NUM) {
		if (msg->magic == htonl(MAGIC_NUM))
			printf("peer endian mismatched. this=0x%x, peer=0x%x\n",
					MAGIC_NUM, msg->magic);
		else
			printf("magic number is not matched. this=0x%x, peer=0x%x\n",
					MAGIC_NUM, msg->magic);
		return -1;
	}

	if (peer->seq_id != msg->seq_id) {
		printf("seq_id is not matched. this=%d, peer=%d\n",
				peer->seq_id, msg->seq_id);
		return -1;
	}

	if (peer->trans_id != msg->trans_id) {
		printf("trans_id is not matched. this=%d, peer=%d\n",
				peer->trans_id, msg->trans_id);
		return -1;
	}

	return 0;
}

/* RX processing for CLINT */
int handle_client_message(peer_t *peer)
{
	convert_ntoh_message_common(peer);
	convert_ntoh_message_each_type(peer);

	if (is_client_invalid_packet(peer)) {
		dump_packet("RX-(invalid)----> ", 
				(char *) &peer->rx_buff, 0, peer->rx_bytes);
		return -1;
	}

	print_message_common(peer);
	process_client_message_each_type(peer);

	inc_trans_id(peer);
	inc_seq_id(peer);

	return 0;
}



/************************************************************
 *
 * TX API for CLINT
 *
 ************************************************************/

int send_data1(peer_t *peer, int a, int b)
{
	msg_type_t type = MSG_TYPE_1;
	data1_t d = { .a = a, .b = b };
	int len = sizeof(d);

	return client_send_msg(peer, type, len, (void *) &d);
}

int send_data2(peer_t *peer, int a, int b, int c)
{
	msg_type_t type = MSG_TYPE_2;
	data2_t d = { .a = a, .b = b, .c = c };
	int len = sizeof(d);

	return client_send_msg(peer, type, len, (void *) &d);
}

int send_data3(peer_t *peer, char *text)
{
	msg_type_t type = MSG_TYPE_3;
	data3_t d;
	int len = sizeof(d);

	memcpy(&d.text[0], text, 32);

	return client_send_msg(peer, type, len, (void *) &d);
}

