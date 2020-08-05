#ifndef DISPLAYER_H_
#define DISPLAYER_H_

#include "msg_info.h"

#include <ncurses.h>
#include <stddef.h>

#define CP_USER		0
#define CP_SERVER	1
#define CP_NOTIF	2
#define CP_ME		3
#define CP_LINE		4
       
struct list_head {
	struct list_node *first;
};

struct list_node {
	struct list_node *next;
	char name[MAX_NAME_LEN];	
};

void init_disp(void);
void destroy_disp(void);
void display_servname(const char * const serv);
void display_msg(const struct msg_info * const msg);
int wait4msg(struct msg_info *msg);
void add_user(const char * const name);
void del_user(const char * const name);

#endif	// DISPLAYER_H_
