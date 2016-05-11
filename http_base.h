#ifndef __HTTP_BASE_H__
#define __HTTP_BASE_H__

#include "util.h"

#define HTTP_REQUEST_ERROR  -10
#define HTTP_METHOD_ERROR   -11
#define HTTP_PROTOCOL_ERROR -12

#define GET     10
#define POST    11
#define HEAD    12
#define PUT     13
#define DELETE  14
#define TRACE   15
#define CONNECT 16
#define OPTIONS 17

typedef struct http_request_s {
    void *root;
    int fd;
    char buf[MAX_BUF];
    void *pos, *last;
    int state;
    void *request_start;
    void *method_start;
    void *method_end;   
    int method;
    void *url_start;
    void *url_end;       
    void *path_start;           // 包含'/'
    void *path_end;
    void *query_start;
    void *query_end;
    void *protocol_start;
    void *protocol_end;
    int http_major;
    int http_minor;
    void *request_end;

    struct list_head list;  /* store http header */
    void *cur_header_key_start;
    void *cur_header_key_end;
    void *cur_header_value_start;
    void *cur_header_value_end;

    void *body_start;

} http_request_t;

typedef struct http_response_s{
    int fd;
    int keep_alive;
    time_t mtime;       /* the modified time of the file*/
    int modified;       /* compare If-modified-since field with mtime to decide whether the file is modified since last time*/

    int status;
} http_response_t;

int init_request_t(http_request_t *req, int fd, conf_t *cf);
int free_request_t(http_request_t *req);

int init_response_t(http_response_t *res, int fd);
int free_response_t(http_response_t *res);

#endif