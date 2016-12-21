#include "http_base.h"

int init_request_t(http_request_t *req, int fd, int coid, conf_t *cft) {
    req->fd = fd;
    req->pos = req->last = req->buf;
    req->state = 0;
    req->root = cft->root;
    memset(req->buf, 0, sizeof(req->buf));

    req->request_start = req->method_start = req->method_end = NULL;
    req->url_start = req->url_end = req->path_start = req->path_end = NULL;
    req->query_start = req->query_end = req->protocol_start = req->protocol_end = NULL;
    req->request_end = NULL;

    INIT_LIST_HEAD(&(req->list));
    req->cur_header_key_start = req->cur_header_key_end = NULL;
    req->cur_header_value_start = req->cur_header_value_end = NULL;

    req->body_start = NULL;
    //req->mtime = time(NULL);
    //req->istimeout = 0;
    req->coid = coid;

    #ifdef _TIMEOUT
    req->timer = NULL;
    #endif

    return DLY_OK;
}

int init_request_t_copy(http_request_t *reqnew, http_request_t *reqold) {
    reqnew->fd = reqold->fd;
    reqnew->pos = reqnew->last = reqnew->buf;
    reqnew->state = 0;
    reqnew->root = reqold->root;
    memset(reqnew->buf, 0, sizeof(reqnew->buf));

    reqnew->request_start = reqnew->method_start = reqnew->method_end = NULL;
    reqnew->url_start = reqnew->url_end = reqnew->path_start = reqnew->path_end = NULL;
    reqnew->query_start = reqnew->query_end = reqnew->protocol_start = reqnew->protocol_end = NULL;
    reqnew->request_end = NULL;

    INIT_LIST_HEAD(&(reqnew->list));
    reqnew->cur_header_key_start = reqnew->cur_header_key_end = NULL;
    reqnew->cur_header_value_start = reqnew->cur_header_value_end = NULL;

    reqnew->body_start = NULL;
    //reqnew->mtime = time(NULL);
    //reqnew->istimeout = 0;
    reqnew->coid = reqold->coid;

    #ifdef _TIMEOUT
    reqnew->timer = NULL;
    #endif
    
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