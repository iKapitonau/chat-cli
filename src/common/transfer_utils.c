#include "transfer_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

void receive_msg(int clientsockfd, struct msg_info *msg)
{
	if (recv(clientsockfd, msg, sizeof(struct msg_info), 0) == -1) {
		perror("recv");
		exit(EXIT_FAILURE);
	}
}

void send_msg(int clientsockfd, const struct msg_info * const msg)
{
	if (send(clientsockfd, msg, sizeof(struct msg_info), 0) == -1) {
		perror("send");
		exit(EXIT_FAILURE);
	}
}
