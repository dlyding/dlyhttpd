#include "http_base.h"

int init_request_t(http_request_t *req, int fd, conf_t *cf) {
    req->fd = fd;
    req->pos = r->last = req->buf;
    req->state = 0;
    req->root = cf->root;
    INIT_LIST_HEAD(&(req->list));

    return ZV_OK;
}

int free_request_t(http_request_t *req) {
    // TODO
    return ZV_OK;
}

int init_response_t(http_response_t *res, int fd) {
    res->fd = fd;
    res->keep_alive = 0;
    res->modified = 1;
    res->status = 0;

    return ZV_OK;
}

int free_response_t(http_response_t *res) {
    // TODO
    return ZV_OK;
}