
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"

/* Reads from stdin and create new message. 
 * This message enqueues to send queueu. */
int read_from_stdin(char *read_buffer, size_t max_len)
{
	int n = 0;
	int tot_n = 0;
	int len;

	memset(read_buffer, 0, max_len);

	n = read(STDIN_FILENO, read_buffer, max_len);
	if (n < 0) {
		if ((errno == EAGAIN) || (errno == EINTR))
			return 0;

		printf("read() failed. err=%d(%s)\n",
				errno, strerror(errno));
		return -1;
	} else if (n > 0) {
		tot_n += n;
		if (tot_n > max_len) {
			printf("Message too large and will be chopped. "\
					"Please try to be shorter next time.\n");
			fflush(STDIN_FILENO);
			return 0;
		}
	}

	return tot_n;
}


/* Reads from stdin and create new message. 
 * This message enqueues to send queueu. */
int handle_read_from_stdin()
{
	int i;
	char stdin_buff[DATA_MAXSIZE]; /* buffer for stdin */
	message_t new_msg;

	if (read_from_stdin(stdin_buff, DATA_MAXSIZE) < 0)
		return -1;

	/* Create new message and enqueue it. */
	memset(&new_msg, 0, sizeof(new_msg));
	prepare_message(SERVER_NAME, stdin_buff, &new_msg);
	print_message(&new_msg);

	/* enqueue message for all clients */
	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i].socket != NO_SOCKET) {
			if (peer_add_to_send(&clients[i], &new_msg) != 0)
				printf("Send buffer was overflowed, we lost this message!\n");
		}
	}

	return 0;
}
