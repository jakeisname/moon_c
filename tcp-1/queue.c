
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "common.h"

int create_message_queue(int queue_size, message_queue_t *queue)
{
	queue->data = calloc(queue_size, sizeof(message_t));
	queue->size = queue_size;
	queue->current = 0;

	return 0;
}

void delete_message_queue(message_queue_t *queue)
{
	free(queue->data);
	queue->data = NULL;
}

int enqueue(message_queue_t *queue, message_t *message)
{
	if (queue->current == queue->size)
		return -1;

	memcpy(&queue->data[queue->current], message, sizeof(message_t));
	queue->current++;

	return 0;
}

int dequeue(message_queue_t *queue, message_t *message)
{
	if (queue->current == 0)
		return -1;

	memcpy(message, &queue->data[queue->current - 1], sizeof(message_t));
	queue->current--;

	return 0;
}

int dequeue_all(message_queue_t *queue)
{
	queue->current = 0;

	return 0;
}
