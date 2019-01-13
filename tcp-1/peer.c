
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

// #define DEBUG_DUMP_PACKET

/* state: 0=possible to request, 1=answer state */
void set_qna_state(peer_t *peer, int state)
{
	peer->qna_state = state;
}

int get_qna_state(peer_t *peer)
{
	return peer->qna_state;
}

void shutdown_properly(int code)
{
	extern peer_t *g_peer;

	delete_peer(g_peer);
	printf("Shutdown client properly.\n");
	exit(code);
}

void disconnect_peer(peer_t *peer)
{
	close(peer->socket);
	dequeue_all(&peer->fifo);
	clear_rx_retry_cnt(peer);
	clear_seq_id(peer);
	set_qna_state(peer, 0);	/* request state */
	peer->tx_bytes = -1;
	peer->rx_bytes = 0;
	printf("Disconeect socket=%d\n", peer->socket);
	printf("\n\n\n");
	peer->socket = NO_SOCKET;
}

int delete_peer(peer_t *peer)
{
	disconnect_peer(peer);
	delete_message_queue(&peer->fifo);
}

int create_peer(peer_t *peer)
{
	create_message_queue(MAX_QUEUE_SIZE, &peer->fifo);

	peer->tx_bytes = -1;
	peer->rx_bytes = 0;

	set_qna_state(peer, 0);	/* request state */

	return 0;
}

char *peer_get_addres_str(peer_t *peer)
{
	static char ret[INET_ADDRSTRLEN + 10];
	char peer_ipv4_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &peer->addres.sin_addr, peer_ipv4_str, INET_ADDRSTRLEN);
	sprintf(ret, "%s(%d)", peer_ipv4_str, ntohs(peer->addres.sin_port));

	return ret;
}

/* server send to client */
int peer_add_to_send(peer_t *peer)
{
	message_t *tx_msg = &peer->tx_buff;
	int rc;

	rc = enqueue(&peer->fifo, tx_msg);

	return 0;
}

int clear_rx_retry_cnt(peer_t *peer)
{
	peer->rx_retry_cnt = 0;
}

int clear_seq_id(peer_t *peer)
{
	peer->seq_id = 0;
}

int inc_seq_id(peer_t *peer)
{
	peer->seq_id++;
}

int inc_trans_id(peer_t *peer)
{
	peer->trans_id++;
}

/* 
 * Receive message from peer and handle it with message_handler(). 
 * 
 * return:	-1:  failed (need to disconnect)
 *		 0:  continue (try again)
 *		 1~: successed (received bytes)
 */
int receive_from_peer(peer_t *peer, int (*message_handler)(peer_t *))
{
	int len;	/* length to receive */
	int n;		/* receive bytes */
	int rc;

	/* Count bytes to receive. */
	len = sizeof(peer->rx_buff) - peer->rx_bytes;

	if (peer->rx_bytes == 0)
		memset(&peer->rx_buff, 0, sizeof(peer->rx_buff));

	/* try to receive bytes */
	n = recv(peer->socket, (char *) &peer->rx_buff + peer->rx_bytes, 
			len, MSG_DONTWAIT);
	if (n < 0) {
		/* peer busy? or server busy? then try again later */
		if (errno == EAGAIN || errno == EINTR)
			return 0;	/* continue */
		/* EPIPE error -> write time only? */
		else if (errno == EPIPE || errno == ETIMEDOUT || errno == ECONNRESET) {
			printf("recv() failed & try to close socket. err=%d(%s)\n",
					errno, strerror(errno));
			return -1;	/* close client socket */
		} else {
			peer->rx_retry_cnt++;
			if (peer->rx_retry_cnt > 3) {
				printf("recv() failed(max-retry) & " \
						"try to close socket. err=%d(%s)\n",
						errno, strerror(errno));
				return -1;	/* close client socket */
			}

			printf("recv() failed. retry=%d, err=%d(%s)\n",
					peer->rx_retry_cnt, errno, strerror(errno));
			return 0;	/* continue */
		}
	} else if (n == 0) {
		/* If recv() returns 0, it means that peer gracefully shutdown. 
		 * Shutdown client. */
		printf("recv() 0 bytes. Peer gracefully shutdown.\n");
		return -1;	/* close client socket */
	}

#ifdef DEBUG_DUMP_PACKET
	dump_packet("RX", (char *) &peer->rx_buff, peer->rx_bytes, n);
#endif

	/* received n bytes */
	clear_rx_retry_cnt(peer);
	peer->rx_bytes += n;

	/* Is completely received? */
	if (peer->rx_bytes >= sizeof(peer->rx_buff)) {
		peer->rx_bytes = 0;
		set_qna_state(peer, 0);

		rc = message_handler(peer);
		if (rc < 0)
			return -1;	/* quit command from client */
	}

	return peer->rx_bytes;
}

/* 
 * Send message to peer. 
 * 
 * return:	-1:  failed (need to disconnect)
 *		 0~: continue (try again)
 *		 1~: successed (sent bytes)
 */
int send_to_peer(peer_t *peer)
{
	int len;	/* lenhth to send */
	int n;	/* send bytes */
	int tot_len;
	int tot_n;

	/* new message then dequeue for tx */
	if (peer->tx_bytes < 0 || peer->tx_bytes >= sizeof(peer->tx_buff)) {
		if (dequeue(&peer->fifo, &peer->tx_buff) != 0) {
			peer->tx_bytes = -1;
			return 0;
		}

		convert_hton_each_data_type(peer);

		/* convert common data last */
		convert_hton_common_data(peer);

		peer->tx_bytes = 0;
	}

	tot_len = sizeof(peer->tx_buff);

	/* Count bytes to send. */
	len = tot_len - peer->tx_bytes;
	if (len <= 0) {
		peer->tx_bytes = -1;
		return 0;
	} 

	n = send(peer->socket, (char *) &peer->tx_buff + peer->tx_bytes, len, 0);
	if (n < 0) {
		/* peer is not ready right now or server is interrupted, try again later. */
		if (errno == EAGAIN || errno == EINTR)
			return 0;
		else if (errno == EPIPE || errno == ETIMEDOUT) {
			printf("send() failed & close socket. err=%d(%s)\n",
					errno, strerror(errno));
			return -1;
		} else {
			printf("send() failed & close socket. err=%d(%s)\n",
					errno, strerror(errno));
			return -1;
		}
	} else if (n == 0)
		/* send 0 bytes. 
                 * It seems that peer can't accept data right now. Try again later. */
		return 0;

#ifdef DEBUG_DUMP_PACKET
	dump_packet("TX", (char *) &peer->tx_buff, peer->tx_bytes, n);
#endif

	/* send n bytes */
	tot_n = peer->tx_bytes;
	peer->tx_bytes += n;


	if (peer->tx_bytes >= tot_len)
		peer->tx_bytes = -1;

	return tot_n;
}

