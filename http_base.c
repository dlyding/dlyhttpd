#include "http_base.h"

int init_request_t(http_request_t *req, int fd, conf_t *cf) {
    req->fd = fd;
    req->pos = req->last = req->buf;
    req->state = 0;
    req->root = cf->root;
    memset(req->buf, 0, sizeof(req->buf));

    req->request_start = req->method_start = req->method_end = NULL;
    req->url_start = req->url_end = req->path_start = req->path_end = NULL;
    req->query_start = req->query_end = req->protocol_start = req->protocol_end = NULL;
    req->request_end = NULL;

    INIT_LIST_HEAD(&(req->list));
    req->cur_header_key_start = req->cur_header_key_end = NULL;
    req->cur_header_value_start = req->cur_header_value_end = NULL;

    req->body_start = NULL;

    return DLY_OK;
}

int free_request_t(http_request_t *req) {
    // TODO
    return DLY_OK;
}

int init_response_t(http_response_t *res, int fd) {
    res->fd = fd;
    res->keep_alive = 0;
    res->modified = 1;
    res->status = HTTP_OK;

    return DLY_OK;
}

int free_response_t(http_response_t *res) {
    // TODO
    return DLY_OK;
}