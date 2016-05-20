#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#include "http_base.h"

#define str3_cmp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define str4_cmp(m, c0, c1, c2, c3)                                        \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

int http_parse_request_line(http_request_t *req);
int http_parse_request_header(http_request_t *req);

#endif