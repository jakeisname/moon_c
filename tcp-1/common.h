#ifndef _COMMON_H_
#define _COMMON_H_ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* max number of clients */
#define MAX_CLIENTS 10

/* Size of send queue (number of messages). */
#define MAX_QUEUE_SIZE 100

/* listen for server */
#define SERVER_IPV4_ADDR "0.0.0.0"

/* message */

typedef enum {
	MSG_TYPE_UNKNOWN,
	MSG_TYPE_1,
	MSG_TYPE_2,
	MSG_TYPE_3,
	MSG_TYPE_MAX,
} msg_type_t;

typedef struct {
#define MAGIC_NUM 0x12345678
	int magic;	/* for endian check */
	int seq_id;
	int trans_id;
	msg_type_t msg_type;
	int msg_len;	/* data length, ex) msg1_t = 8 */
#define MAX_DATA_SIZE 64
	char data[MAX_DATA_SIZE];
} message_t;

typedef struct {
	int a;
	int b;
} data1_t;

typedef struct {
	int a;
	int b;
	int c;
} data2_t;

typedef struct {
#define MAX_TEXT_SIZE 32
	char text[MAX_TEXT_SIZE];
} data3_t;

/* message queue */

typedef struct {
	int size;
	message_t *data;
	int current;
} message_queue_t;


/* peer */

typedef struct {
#define NO_SOCKET -1
	int socket;	/* -1: not connected */
	struct sockaddr_in addres;
	int qna_state;	/* 0=possible to request, 1=wait for answer */
			/* used for client only */

	/* Messages that waiting for send. */
	message_queue_t fifo;

	int seq_id;
	int trans_id;

	/* Buffered sending message.
	 * 
	 * In case we doesn't send whole message per one call send().
	 * And current_sending_byte is a pointer to the part of data that will be send next call.
	 */
	message_t tx_buff;
	int tx_bytes;
	int tx_retry_cnt;	/* retry counter for tx */

	/* The same for the receiving message. */
	message_t rx_buff;
	int rx_bytes;
	int rx_retry_cnt;	/* retry counter for rx */
} peer_t;


extern peer_t server;
extern peer_t clients[MAX_CLIENTS];
extern peer_t *g_peer;

/* option.c */
extern int set_reuseaddr_opt(int fd);
extern int set_sock_nonblocking(int fd);
extern int set_sock_timeout(int fd, int sec);
extern void set_sock_keepallive(int fd, int idle, int interval, int cnt);
extern void set_sock_nodelay(int fd);

/* msg.c */
extern void convert_hton_common_data(peer_t *peer);
extern void convert_hton_each_data_type(peer_t *peer);
extern int handle_server_message(peer_t *peer);
extern int handle_client_message(peer_t *peer);
extern int send_data1(peer_t *peer, int a, int b);
extern int send_data2(peer_t *peer, int a, int b, int c);
extern int send_data3(peer_t *peer, char *text);

/* peer.c */
extern void set_qna_state(peer_t *peer, int state);
extern int get_qna_state(peer_t *peer);
extern void shutdown_properly(int code);
extern void disconnect_peer(peer_t *peer);
extern int delete_peer(peer_t *peer);
extern int create_peer(peer_t *peer);
extern char *peer_get_addres_str(peer_t *peer);
extern int peer_add_to_send(peer_t *peer);
extern int clear_rx_retry_cnt(peer_t *peer);
extern int clear_seq_id(peer_t *peer);
extern int inc_seq_id(peer_t *peer);
extern int inc_trans_id(peer_t *peer);
extern int receive_from_peer(peer_t *peer, int (*server_message_handler)(peer_t *));
extern int send_to_peer(peer_t *peer);

/* queue.c */
extern int create_message_queue(int queue_size, message_queue_t *queue);
extern void delete_message_queue(message_queue_t *queue);
extern int enqueue(message_queue_t *queue, message_t *message);
extern int dequeue(message_queue_t *queue, message_t *message);
extern int dequeue_all(message_queue_t *queue);

/* server.c */
extern int start_listen_socket(int port, int *listen_sock);
extern int get_client_name(int argc, char **argv, char *client_name);
extern int do_server();

/* client.c */
extern int do_client(peer_t *client, char *server_ip, int server_port);

/* signal.c */
extern void handle_signal_action(int sig_number);
extern int setup_signals();

/* stdin.c */
extern int read_from_stdin(char *read_buffer, size_t max_len);
extern int handle_read_from_stdin();

/* debug.c */
extern void dump_packet(char *tx_rx, char *p, int start, int len);

#endif /* _COMMON_H */

