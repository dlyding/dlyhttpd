
#ifndef __TIMER_H__
#define __TIMER_H__

#include "heap.h"

#define TIMER_INFINITE -1
#define TIMEOUT_DEFAULT 5000     /* ms */

//typedef int (*timer_handler_ptr)(void *rq);

typedef struct timer_node_s{
    size_t key;
    int istimeout;    /* if remote client close the socket first, set istimeout to 1 */
    //timer_handler_pt handler;
    //http_request_t *rq;
    void *ud;
} timer_node_t;

int timer_init();
int get_timeout_node_time();
timer_node_t* handle_timeout_node();
int timer_close();

//extern heap_t* heap_timer;
//extern size_t current_msec;

timer_node_t* add_timer(void *ud, size_t timeout);
//void del_timer(void *rq);

#endif
