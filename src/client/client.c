#include "displayer.h"
#include "transfer_utils.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT_SIZE	10

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr);
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int create_client_socket(const char *hostname, const char *port)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int gai_ret;
	struct addrinfo *res;
	if ((gai_ret = getaddrinfo(hostname, port, &hints, &res)) != 0) {
		fprintf(stderr, "CLIENT: getaddrinfo: %s\n", gai_strerror(gai_ret));
		exit(EXIT_FAILURE);
	}

	struct addrinfo *p;
	int sockfd;
	for (p = res; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("CLIENT: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("CLIENT: connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "CLIENT: failed to connect!\n");
		exit(EXIT_FAILURE);
	} else {
		char s[INET6_ADDRSTRLEN];
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
		fprintf(stderr, "CLIENT: connected to %s\n", s);
	}

	freeaddrinfo(res);

	return sockfd;
}

static pthread_mutex_t lock;
static int clientsockfd;

void *receiver_entry(void *args)
{
	struct msg_info msg;
	while (1) {
		receive_msg(clientsockfd, &msg);

		pthread_mutex_lock(&lock);
		display_msg(&msg);
		pthread_mutex_unlock(&lock);
	}
}

void *transmitter_entry(void *args)
{
	while (1) {
		struct msg_info msg;
		wait4msg(&msg);
		send_msg(clientsockfd, &msg);

		pthread_mutex_lock(&lock);
		display_msg(&msg);
		pthread_mutex_unlock(&lock);
	}
}

int main(void)
{
	char servname[BUF_SIZE];
	printf("Server IP: ");
	scanf("%s", servname);

	char port[PORT_SIZE];
	printf("Server port: ");
	scanf("%s", port);

	clientsockfd = create_client_socket(servname, port);

	init_disp();
	display_servname(servname);

	struct msg_info msg;
	receive_msg(clientsockfd, &msg);
	display_msg(&msg);

	struct msg_info name;
	wait4msg(&name);
	display_msg(&name);

	send_msg(clientsockfd, &name);
	receive_msg(clientsockfd, &msg);
	display_msg(&msg);

	if (pthread_mutex_init(&lock, NULL) != 0) {
		fprintf(stderr, "Mutex init has failed!\n");
		return EXIT_FAILURE;
	}

	pthread_t receiver, transmitter;
	if (pthread_create(&receiver, NULL, receiver_entry, NULL)) {
		fprintf(stderr, "Creating receiver thread has failed!\n");
		return EXIT_FAILURE;
	}
	if (pthread_create(&transmitter, NULL, transmitter_entry, NULL)) {
		fprintf(stderr, "Creating transmitter thread has failed!\n");
		return EXIT_FAILURE;
	}
	
	pthread_join(receiver, NULL);
	pthread_join(transmitter, NULL);
	pthread_mutex_destroy(&lock);

	close(clientsockfd);

	destroy_disp();

	return EXIT_SUCCESS;
}
