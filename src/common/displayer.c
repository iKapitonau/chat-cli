#include "displayer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _clear_printing_area(void);
static void _clear_chat_area(void);
static void _clear_users_area(void);
static void _push_msg(const struct msg_info * const msg);
static void _update_users(void);
static void _display_msgs(void);
static inline int _msgt_to_cp(enum msg_type msgt);

static struct msg_info cur_msg;

static struct msg_info msg_hist[MAX_MSG_HIST];
static size_t first_msg;
static size_t hist_size;

static struct list_head online_users;

static WINDOW *users_area;
static WINDOW *printing_area;
static WINDOW *chat_area;
static WINDOW *servername;

void init_disp(void)
{
	initscr();
	start_color();
	noecho();
	refresh();

	init_color(COLOR_RED, 1000, 0, 0);
	init_color(COLOR_YELLOW, 1000, 1000, 0);
	init_color(COLOR_MAGENTA, 1000, 0, 1000);

	init_pair(CP_SERVER, COLOR_RED, 0);
	init_pair(CP_NOTIF, COLOR_YELLOW, 0);
	init_pair(CP_ME, COLOR_MAGENTA, 0);
	init_pair(CP_LINE, COLOR_GREEN, 0);

	first_msg = hist_size = 0;
	online_users.first = NULL;

	users_area = newwin(LINES - 3, COLS / 4, 3, COLS - COLS / 4);
	printing_area = newwin(3, COLS - COLS / 4, LINES - 3, 0);
	chat_area = newwin(LINES - 3, COLS - COLS / 4, 0, 0);
	servername = newwin(3, COLS / 4, 0, COLS - COLS / 4);
	
	wattron(servername, COLOR_PAIR(CP_LINE));
	box(servername, 0, 0);
	wattroff(servername, COLOR_PAIR(CP_LINE));

	_clear_users_area();
	_clear_chat_area();
	_clear_printing_area();

	wrefresh(servername);
	wrefresh(users_area);
	wrefresh(chat_area);
	wrefresh(printing_area);
}

void destroy_disp(void)
{
	delwin(users_area);
	delwin(printing_area);
	delwin(chat_area);
	delwin(servername);

	endwin();
}

void display_servname(const char * const serv)
{
	mvwprintw(servername, 1, 1, "Server: %s", serv);
	wrefresh(servername);
}

void display_msg(const struct msg_info * const msg)
{
	_push_msg(msg);
	_display_msgs();
}

int wait4msg(struct msg_info *msg)
{
	while (1) {
		int c = getch();	
		if (!iscntrl(c)) {
			if (cur_msg.msg_len < BUF_SIZE) {
				cur_msg.msg[cur_msg.msg_len++] = c;
				cur_msg.msg[cur_msg.msg_len] = '\0';
				wattron(printing_area, COLOR_PAIR(CP_ME));
				waddch(printing_area, c);
				wrefresh(printing_area);
			}
		} else if (c == '\n') {
			if (cur_msg.msg_len > 0) {
				strcpy(msg->msg, cur_msg.msg);
				msg->msg_len = cur_msg.msg_len;
				strcpy(msg->sender, "You");
				msg->msg[cur_msg.msg_len] = '\0';
				msg->sender_len = sizeof("You");
				msg->type = MSGT_ME;

				_clear_printing_area();
				cur_msg.msg_len = 0;
				cur_msg.msg[cur_msg.msg_len] = '\0';
				return 0;
			}
			return 1;
		} else if (c == 8 || c == 127) {
			if (cur_msg.msg_len > 0) {
				--cur_msg.msg_len;
				cur_msg.msg[cur_msg.msg_len] = '\0';
				int y, x;
				getyx(printing_area, y, x);
				wmove(printing_area, y, x - 1);
				waddch(printing_area, ' ');
				wmove(printing_area, y, x - 1);
				wrefresh(printing_area);
			}
		}
	}
}

void add_user(const char * const name)
{
	struct list_node *tmp = malloc(sizeof(struct list_node));
	tmp->next = online_users.first;
	strcpy(tmp->name, name);
	online_users.first = tmp;
	_update_users();
}

void del_user(const char * const name)
{
	void *prev = &online_users;
	struct list_node *cur;
	for (cur = online_users.first; cur != NULL; cur = cur->next) {
		if (strcmp(cur->name, name) == 0)
			break;
		prev = cur;	
	}

	if (prev == &online_users)
		((struct list_head *)prev)->first = cur->next;
	else
		((struct list_node *)prev)->next = cur->next;

	free(cur);
	_update_users();
}

static void _clear_printing_area(void)
{
	wclear(printing_area);

	wattron(printing_area, COLOR_PAIR(CP_LINE));
	box(printing_area, 0, 0);
	wattroff(printing_area, COLOR_PAIR(CP_LINE));
	
	wattron(printing_area, COLOR_PAIR(CP_ME));
	mvwaddstr(printing_area, 1, 1, "[You]: ");

	wrefresh(printing_area);
}

static void _clear_chat_area(void)
{
	wclear(chat_area);

	wattron(chat_area, COLOR_PAIR(CP_LINE));
	box(chat_area, 0, 0);
	wattroff(chat_area, COLOR_PAIR(CP_LINE));

	wrefresh(chat_area);
}

static void _clear_users_area(void)
{
	wclear(users_area);

	wattron(users_area, COLOR_PAIR(CP_LINE));
	box(users_area, 0, 0);
	wattroff(users_area, COLOR_PAIR(CP_LINE));

	wrefresh(users_area);
}

static void _push_msg(const struct msg_info * const msg)
{
	int idx;

	if (hist_size < MAX_MSG_HIST) {
		idx = hist_size;
		++hist_size;
	} else {
		idx = first_msg;
		++first_msg;
	}

	memcpy(&msg_hist[idx], msg, sizeof(struct msg_info));
}

static void _update_users(void)
{
	_clear_users_area();	
	mvwprintw(users_area, 1, 1, "Online:");

	int row = 2;
	for (struct list_node *cur = online_users.first;
		cur != NULL; cur = cur->next) {
		mvwprintw(users_area, row, 1, "%s", cur->name);
		++row;
	}

	wrefresh(users_area);
}

static void _display_msgs(void)
{
	if (hist_size == 0)
		return;

	_clear_chat_area();

	int rows, cols;
	getmaxyx(chat_area, rows, cols);

	int last_msg = (hist_size == MAX_MSG_HIST ? 
		(first_msg - 1 + MAX_MSG_HIST) % MAX_MSG_HIST :
		hist_size - 1);

	int i = last_msg;
	for (ssize_t row = rows - 2; row > 0; --row) {
		wattron(chat_area, COLOR_PAIR(_msgt_to_cp(msg_hist[i].type)));
		if (msg_hist[i].type == MSGT_NOTIF)
			mvwprintw(chat_area, row, 1, "%s", msg_hist[i].msg);
		else
			mvwprintw(chat_area, row, 1, "[%s]: %s", msg_hist[i].sender, msg_hist[i].msg);
		wattroff(chat_area, COLOR_PAIR(_msgt_to_cp(msg_hist[i].type)));
		if (i == first_msg)
			break;
		--i;
		if (i == -1)
			i = MAX_MSG_HIST - 1;
	}
	wrefresh(chat_area);
}

static inline int _msgt_to_cp(enum msg_type msgt) {
	int res = CP_USER;

	if (msgt == MSGT_SERVER)
		res = CP_SERVER;
	else if (msgt == MSGT_NOTIF)
		res = CP_NOTIF;
	else if (msgt == MSGT_ME)
		res = CP_ME;

	return res;
}
