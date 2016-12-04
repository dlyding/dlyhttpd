#ifndef _EPOLL_H
#define _EPOLL_H

#include <sys/epoll.h>

typedef struct epoll_event epoll_event_t;

typedef struct epoll_s {
	int epfd;
	int maxevents;
	epoll_event_t* events;
} epoll_t;

epoll_t* epoll_create(int size, int maxevents);
void epoll_add(epoll_t* et, int fd, void* ptr, __uint32_t flag_bit);
void epoll_mod(epoll_t* et, int fd, void* ptr, __uint32_t flag_bit);
void epoll_del(epoll_t* et, int fd);
int epoll_wait1(epoll_t* et, int timeout);
void epoll_close(epoll_t* et);


#endif
