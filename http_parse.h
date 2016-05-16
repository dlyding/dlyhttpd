#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#include "util.h"
#include "http_base.h"

#define str3_cmp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define str4_cmp(m, c0, c1, c2, c3)                                        \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

typedef struct http_header_s {
    char *key_start, *key_end;          /* not include end */
    char *value_start, *value_end;
    list_head list;
} http_header_t;

typedef int (*http_header_handler_pt)(http_request_t *req, http_response_t *res, char *data, int len);

typedef struct http_header_handle_s{
    char *name;
    http_header_handler_pt handler;
} http_header_handle_t;

int http_parse_request_line(http_request_t *req);
int http_parse_request_header(http_request_t *req);

#endif