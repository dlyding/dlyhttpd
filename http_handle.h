#ifndef __HTTP_HANDLE_H__
#define __HTTP_HANDLE_H__

#include "http_base.h"
#include <string.h>

typedef struct mime_type_s {
	const char *type;
	const char *value;
} mime_type_t;

int set_method_for_request(http_request_t *req);
int set_protocol_for_request(http_request_t *req);
int set_url_for_request(http_request_t *req);
const char* get_file_type(const char *type);
int get_information_from_url(const http_request_t *req, char *filename, char *querystring);
void http_handle_header(http_request_t *req, http_response_t *res);

#endif
