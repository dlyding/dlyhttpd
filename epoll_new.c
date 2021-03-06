#include "epoll_new.h"
#include "dbg.h"

epoll_t* epoll_create_new(int size, int maxevents) {
    epoll_t* et = (epoll_t*)malloc(sizeof(epoll_t));
    et->epfd = epoll_create1(size);
    et->maxevents = maxevents; 
    check(et->epfd > 0, "epoll_create");

    et->events = (epoll_event_t *)malloc(sizeof(epoll_event_t) * et->maxevents);
    return et;
}

void epoll_add(epoll_t* et, int fd, void* ptr, __uint32_t flag_bit) {
    epoll_event_t event;
    //event.data.fd = fd;    // epoll_event_t 是联合体
    event.data.ptr = ptr;
    event.events = flag_bit;
    int rc = epoll_ctl(et->epfd, EPOLL_CTL_ADD, fd, &event);
    check(rc == 0, "epoll_add");
    return;
}

void epoll_add_para(epoll_t* et, int fd, int para, __uint32_t flag_bit) {
    epoll_event_t event;
    //event.data.fd = fd;    // epoll_event_t 是联合体
    //event.data.ptr = ptr;
    event.data.fd = para;
    event.events = flag_bit;
    int rc = epoll_ctl(et->epfd, EPOLL_CTL_ADD, fd, &event);
    check(rc == 0, "epoll_add");
    return;
}

void epoll_mod(epoll_t* et, int fd, void* ptr, __uint32_t flag_bit) {
    epoll_event_t event;
    //event.data.fd = fd;
    event.data.ptr = ptr;
    event.events = flag_bit;
    int rc = epoll_ctl(et->epfd, EPOLL_CTL_MOD, fd, &event);
    check(rc == 0, "epoll_mod");
    return;
}

void epoll_mod_para(epoll_t* et, int fd, int para, __uint32_t flag_bit) {
    epoll_event_t event;
    //event.data.fd = fd;
    //event.data.ptr = ptr;
    event.data.fd = para;
    event.events = flag_bit;
    int rc = epoll_ctl(et->epfd, EPOLL_CTL_MOD, fd, &event);
    check(rc == 0, "epoll_mod");
    return;
}

void epoll_del(epoll_t* et, int fd) {
    //epoll_event_t event;
    /*et->event.data.fd = fd;
    et->event.data.ptr = NULL;
    rt->event.events = 0;*/
    int rc = epoll_ctl(et->epfd, EPOLL_CTL_DEL, fd, NULL);
    check(rc == 0, "epoll_del");
    return;
}

int epoll_wait_new(epoll_t* et, int timeout) {
    int n;
again:
    n = epoll_wait(et->epfd, et->events, et->maxevents, timeout);
    if(n < 0 && errno == EINTR)
        goto again;
    check(n >= 0, "epoll_wait");
    return n;
}

void epoll_close_new(epoll_t* et) {
    free(et->events);
    close(et->epfd);
    free(et);
}
