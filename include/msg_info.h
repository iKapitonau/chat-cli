#ifndef MSG_INFO_H_
#define MSG_INFO_H_

#include <stddef.h>

#define MAX_MSG_LEN	256
#define MAX_MSG_HIST	1000
#define MAX_NAME_LEN	20
#define BUF_SIZE	MAX_NAME_LEN + MAX_MSG_LEN + 10

enum msg_type { MSGT_USER, MSGT_SERVER, MSGT_NOTIF, MSGT_ME };

struct msg_info {
	char msg[MAX_MSG_LEN];
	char sender[MAX_NAME_LEN];
	size_t msg_len;	
	size_t sender_len;
	enum msg_type type;
};

#endif	// MSG_INFO_H_
