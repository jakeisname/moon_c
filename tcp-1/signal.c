
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"

void handle_signal_action(int sig_number)
{
	if (sig_number == SIGINT) {
		printf("SIGINT was catched!\n");
		shutdown_properly(EXIT_SUCCESS);
	}
	else if (sig_number == SIGPIPE)
		printf("SIGPIPE was catched!\n");
}

int setup_signals()
{
	struct sigaction sa;
	sa.sa_handler = handle_signal_action;
	if (sigaction(SIGINT, &sa, 0) != 0) {
		perror("sigaction()");
		return -1;
	}
	if (sigaction(SIGPIPE, &sa, 0) != 0) {
		perror("sigaction()");
		return -1;
	}

	return 0;
}
