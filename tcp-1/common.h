#ifndef _COMMON_H_
#define _COMMON_H_ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* max number of clients */
#define MAX_CLIENTS 10

/* server name for display */
#define SERVER_NAME "orange"

/* socket number for un-connected status */
#define NO_SOCKET -1

/* Maximum bytes that can be send() or recv() via net by one call.
 * It's a good idea to test sending one byte by one.
 */
#define MAX_SEND_SIZE 100

/* Size of send queue (messages). */
#define MAX_MESSAGES_BUFFER_SIZE 100

#define SENDER_MAXSIZE 10
#define DATA_MAXSIZE 512

/* for listen */
#define SERVER_IPV4_ADDR "127.0.0.1"

/* message */

typedef struct {
	char data[DATA_MAXSIZE];
	char sender[SENDER_MAXSIZE];
}  message_t;


/* message queue */

typedef struct {
	int size;
	message_t *data;
	int current;
} message_queue_t;


/* peer */

typedef struct {
	int socket;
	struct sockaddr_in addres;

	/* Messages that waiting for send. */
	message_queue_t fifo;

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

/* option.c */
int set_reuseaddr_opt(int fd);
int set_sock_nonblocking(int fd);
int set_sock_timeout(int fd, int sec);
void set_sock_keepallive(int fd, int idle, int interval, int cnt);

/* msg.c */
extern int prepare_message(char *sender, char *data, message_t *message);
extern int print_message(message_t *message);
extern int handle_received_message(peer_t *peer);

/* peer.c */
extern int delete_peer(peer_t *peer);
extern int create_peer(peer_t *peer);
extern char *peer_get_addres_str(peer_t *peer);
extern int peer_add_to_send(peer_t *peer, message_t *message);
extern int receive_from_peer(peer_t *peer, int (*message_handler)(peer_t *));
extern int send_to_peer(peer_t *peer);

/* queue.h */
extern int create_message_queue(int queue_size, message_queue_t *queue);
extern void delete_message_queue(message_queue_t *queue);
extern int enqueue(message_queue_t *queue, message_t *message);
extern int dequeue(message_queue_t *queue, message_t *message);
extern int dequeue_all(message_queue_t *queue);

/* server.c */
extern int start_listen_socket(int port, int *listen_sock);
extern int handle_new_connection();
extern int close_client_connection(peer_t *client);
extern int get_client_name(int argc, char **argv, char *client_name);
extern int build_fd_sets(fd_set *read_fds, fd_set *write_fds, fd_set *except_fds);
extern void shutdown_properly(int code);
extern int do_server();

/* client.c */
extern int connect_server(peer_t *server);
extern int build_fd_sets_for_client(peer_t *server, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds);

/* signal.c */
extern void handle_signal_action(int sig_number);
extern int setup_signals();

/* stdin.c */
extern int read_from_stdin(char *read_buffer, size_t max_len);
extern int handle_read_from_stdin();

#endif /* _COMMON_H */
