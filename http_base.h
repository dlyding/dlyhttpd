#ifndef __HTTP_BASE_H__
#define __HTTP_BASE_H__

#include "util.h"
#include "list.h"

// HTTP协议分析错误码
#define HTTP_REQUEST_ERROR  -10
#define HTTP_METHOD_ERROR   -11
#define HTTP_PROTOCOL_ERROR -12
#define HTTP_PATH_ERROR     -13
#define HTTP_HEAD_ERROR     -14

// HTTP状态码
#define HTTP_OK             200
#define HTTP_NOT_MODIFIED   304
#define HTTP_FORBIDDEN      403
#define HTTP_NOT_FOUND      404

// HTTP请求方法
#define GET     10
#define POST    11
#define HEAD    12
#define PUT     13
#define DELETE  14
#define TRACE   15
#define CONNECT 16
#define OPTIONS 17

#define MAX_BUF     10240
#define MAXLINE     8192
#define SHORTLINE   512
#define METHODLEN   15

typedef struct http_request_s {
    char *root;                  // 不包含'/'
    int fd;
    char buf[MAX_BUF];
    char *pos, *last;
    int state;
    char *request_start;
    char *method_start;
    char *method_end;   
    int method;
    char *url_start;
    char *url_end;       
    char *path_start;           // 包含'/'
    char *path_end;
    char *query_start;
    char *query_end;
    char *protocol_start;
    char *protocol_end;
    int http_major;
    int http_minor;
    char *request_end;

    struct list_head list;  /* store http header */
    char *cur_header_key_start;
    char *cur_header_key_end;
    char *cur_header_value_start;
    char *cur_header_value_end;

    char *body_start;
    int body_length;

    int istimeout;      // 判断是否超时
    time_t mtime;       // 保存请求时的时间，用于超时

} http_request_t;

typedef struct http_response_s{
    int fd;
    int keep_alive;
    time_t mtime;       /* the modified time of the file*/
    int modified;       /* compare If-modified-since field with mtime to decide whether the file is modified since last time*/

    int status;
} http_response_t;

typedef struct http_header_s {
    char *key_start, *key_end;         
    char *value_start, *value_end;
    list_head list;
} http_header_t;

int init_request_t(http_request_t *req, int fd, conf_t *cf);
int free_request_t(http_request_t *req);

int init_response_t(http_response_t *res, int fd);
int free_response_t(http_response_t *res);

#endif