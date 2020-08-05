#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "transfer_utils.h"

#define PORT		"9999"
#define BACKLOG		10

int create_listener_socket(void)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int gai_ret;
	struct addrinfo *res;
	if ((gai_ret = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "[Server]: getaddrinfo: %s\n", gai_strerror(gai_ret));
		exit(EXIT_FAILURE);
	}

	struct addrinfo *p;
	int sockfd, yes = 1;
	for (p = res; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("[Server]: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(&yes)) == -1) {
			close(sockfd);
			perror("[Server]: setsockopt");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("[Server]: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(res);

	if (p == NULL) {
		fprintf(stderr, "[Server]: failed to bind!\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		close(sockfd);
		perror("[Server]: listen");
		exit(EXIT_FAILURE);
	}

	return sockfd;
}

struct pollfd pfds[BACKLOG + 1];
int fd_count;

int register_sock(int sockfd)
{
	if (fd_count == BACKLOG + 1)
		return -1;
	pfds[fd_count].fd = sockfd;
	pfds[fd_count].events = POLLIN;
	fd_count++;
	return 0;
}

void unregister_sock(int sock_idx)
{
	fd_count--;
	pfds[sock_idx] = pfds[fd_count];
}

void deny(int clientfd)
{
	static const char* const txt = "Sorry, no place for you :(";
	static struct msg_info msg;
	strcpy(msg.msg, txt);
	msg.msg_len = strlen(txt);
	strcpy(msg.sender, "Server");
	msg.sender_len = strlen(msg.sender);
	msg.type = MSGT_SERVER;
	send_msg(clientfd, &msg);
}

char names[BUF_SIZE][MAX_NAME_LEN];

void register_client(int clientfd)
{
	static const char * const askname = "Your name?";

	static struct msg_info msg;
	strcpy(msg.msg, askname);
	msg.msg_len = strlen(askname);
	strcpy(msg.sender, "Server");
	msg.sender_len = strlen(msg.sender);
	msg.type = MSGT_SERVER;

	send_msg(clientfd, &msg);

	static struct msg_info ans;
	receive_msg(clientfd, &ans);
	fprintf(stderr, "[Server]: %d -- %s\n", clientfd, ans.msg);
	strcpy(names[clientfd], ans.msg);

	msg.msg_len = sprintf(msg.msg, "Greetings, %s!", ans.msg);
	send_msg(clientfd, &msg);
}

/*
void send_names(int clientfd)
{
	char msg[BUF_SIZE];
	sprintf(msg, "[Server]: %d user(s) online: ", fd_count - 1);
	send_msg(clientfd, msg, strlen(msg));
	int first = 1;
	for (int i = 1; i < fd_count; ++i) {
		if (first) {
			msg[0] = '\0';
			first = 0;
		} else
			strcpy(msg, ", ");
		strcat(msg, names[pfds[i].fd]);
		send_msg(clientfd, msg, strlen(msg));
	}
}
*/

int main(void)
{
	int listener = create_listener_socket();
	register_sock(listener);

	fprintf(stderr, "[Server]: ready to access clients...\n");
	
	struct sockaddr client_addr;
	socklen_t addr_len;
	int clientfd;
	while (1) {
		int poll_count = poll(pfds, fd_count, -1);
		if (poll_count == -1) {
			perror("[Server]: poll");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < fd_count; ++i) {
			if (pfds[i].revents & POLLIN) {
				if (pfds[i].fd == listener) {
					if ((clientfd = accept(listener, &client_addr, &addr_len)) == -1) {
						perror("[Server]: accept");
						continue;
					}
					if (register_sock(clientfd) == -1) {
						deny(clientfd);
						close(clientfd);	
					} else {
						register_client(clientfd);

						struct msg_info msg;
						msg.msg_len = sprintf(msg.msg, "%s has just entered the chat.", names[clientfd]);
						msg.type = MSGT_NOTIF;
						for (int j = 0; j < fd_count; ++j) {
							int dest_fd = pfds[j].fd;
							if (dest_fd != listener && dest_fd != clientfd)
								send_msg(dest_fd, &msg);
						}
						//send_names(clientfd);
					}
				} else {
					struct msg_info msg;
					receive_msg(pfds[i].fd, &msg);
					int nbytes = msg.msg_len;
					int sender_fd = pfds[i].fd;

					if (nbytes <= 0) {
						if (nbytes == 0)
							fprintf(stderr, "[Server]: socket %d hung up\n", sender_fd);
						else
							perror("[Server]: poll");
						close(pfds[i].fd);
						unregister_sock(i);
					} else {
						for (int j = 0; j < fd_count; ++j) {
							int dest_fd = pfds[j].fd;
							if (dest_fd != listener && dest_fd != sender_fd) {
								strcpy(msg.sender, names[sender_fd]);
								msg.sender_len = strlen(names[sender_fd]);
								msg.type = MSGT_USER;

								send_msg(dest_fd, &msg);
							}
						}
					}
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
