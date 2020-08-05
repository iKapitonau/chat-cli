#ifndef TRANSFER_UTILS_H_
#define TRANSFER_UTILS_H_

#include "msg_info.h"

void receive_msg(int clientsockfd, struct msg_info *msg);
void send_msg(int clientsockfd, const struct msg_info * const msg);

#endif	// TRANSFER_UTILS_H_
