
#include <sys/time.h>
#include "timer.h"

static int timer_comp(void *ti, void *tj) {
    timer_node_t *timeri = (timer_node_t *)ti;
    timer_node_t *timerj = (timer_node_t *)tj;

    return (timeri->key < timerj->key)? 1: 0;
}

heap_t* heap_timer = NULL;
size_t current_msec;

static void time_update() {
    // there is only one thread calling time_update, no need to lock?
    struct timeval tv;
    int rc;

    rc = gettimeofday(&tv, NULL);
    check(rc == 0, "time_update: gettimeofday error");

    current_msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    debug("in time_update, time = %zu", current_msec);
}


int timer_init() {
    int rc;
    if(heap_timer != NULL) 
        return -1;
    heap_timer = heap_create(timer_comp, HEAP_DEFAULT_SIZE);
    check(heap_timer != NULL, "heap_create error");
   
    time_update();
    return DLY_OK;
}

int get_timeout_node_time() {
    timer_node_t *tn;
    int time = TIMER_INFINITE;
    int rc;

    while (!heap_is_empty(heap_timer)) {
        debug("get_timeout_node_time");
        time_update();
        tn = (timer_node_t *)heap_top(heap_timer);
        check(tn != NULL, "heap_top error");

        if (tn->istimeout) {
            rc = heap_deltop(heap_timer); 
            check(rc == 0, "heap_deltop");
            free(tn);
            continue;
        }
             
        time = (int) (tn->key - current_msec);
        debug("in get_timeout_node_time, key = %zu, cur = %zu",
                tn->key,
                current_msec);
        time = (time > 0? time: 0);
        break;
    }
    return time;
}

timer_node_t* handle_timeout_node() {
    debug("in handle_timeout_node");
    timer_node_t *tn;
    int rc;

    while (!heap_is_empty(heap_timer)) {
        debug("handle_timeout_node, size = %zu", heap_size(heap_timer));
        time_update();
        tn = (timer_node_t *)heap_top(heap_timer);
        check(tn != NULL, "heap_top error");

        if (tn->istimeout) {
            rc = heap_deltop(heap_timer); 
            check(rc == 0, "handle_timeout_node: zv_pq_delmin error");
            free(tn);
            continue;
        }
        
        if (tn->key > current_msec) {
            return NULL;
        }

        /*if (tn->handler) {
            tn->handler(tn->rq);
        }*/
        tn->istimeout = 1;
        return tn;
        /*rc = heap_deltop(&heap_timer); 
        check(rc == 0, "handle_timeout_node: zv_pq_delmin error");
        free(tn);*/
    }
    return NULL;
}

timer_node_t* add_timer(void *ud, size_t timeout) {
    int rc;
    timer_node_t *tn = (timer_node_t *)malloc(sizeof(timer_node_t));
    check(tn != NULL, "zv_add_timer: malloc error");

    time_update();
    //rq->timer = tn;
    tn->key = current_msec + timeout;
    debug("in zv_add_timer, key = %zu", tn->key);
    tn->istimeout = 0;
    //tn->handler = handler;
    tn->ud = ud;

    rc = heap_insert(heap_timer, tn);
    check(rc == 0, "zv_add_timer: zv_pq_insert error");
    return tn;
}

/*void del_timer(http_request_t *rq) {
    debug("in zv_del_timer");
    time_update();
    timer_node_t *tn = rq->timer;
    check(tn != NULL, "zv_del_timer: rq->timer is NULL");

    tn->istimeout = 1;
}*/

int timer_close() {
    heap_close(heap_timer);
    return DLY_OK;
}
