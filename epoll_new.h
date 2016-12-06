#ifndef __EPOLL_H__
#define __EPOLL_H__

#include <sys/epoll.h>
#include <unistd.h>

typedef struct epoll_event epoll_event_t;

typedef struct epoll_s {
	int epfd;
	int maxevents;
	epoll_event_t* events;
} epoll_t;

epoll_t* epoll_create_new(int size, int maxevents);
void epoll_add(epoll_t* et, int fd, void* ptr, __uint32_t flag_bit);
void epoll_mod(epoll_t* et, int fd, void* ptr, __uint32_t flag_bit);
void epoll_del(epoll_t* et, int fd);
int epoll_wait_new(epoll_t* et, int timeout);
void epoll_close_new(epoll_t* et);


#endif
