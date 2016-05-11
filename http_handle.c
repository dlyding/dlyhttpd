#include "http_handle.h"

int set_method_for_request(http_request_t *req)
{
	char met[10], *p;
	int i = 0;
	p = req->method_start;
	while(p <= req->method_end) {
		if(*p == ' ') {
			p++;
			continue;
		}
		met[i] = *p;
		p++;
		i++;
	}
	met[i] = '\0';
	if(strcmp(met, "GET") == 0) {
		req->method = GET;
		return DLY_OK;
	}

	if(strcmp(met, "POST") == 0) {
		req->method = POST;
		return DLY_OK;
	}

	if(strcmp(met, "HEAD") == 0) {
		req->method = HEAD;
		return DLY_OK;
	}

	if(strcmp(met, "PUT") == 0) {
		req->method = PUT;
		return DLY_OK;
	}

	if(strcmp(met, "DELETE") == 0) {
		req->method = DELETE;
		return DLY_OK;
	}

	if(strcmp(met, "TRACE") == 0) {
		req->method = TRACE;
		return DLY_OK;
	}

	if(strcmp(met, "CONNECT") == 0) {
		req->method = CONNECT;
		return DLY_OK;
	}

	if(strcmp(met, "OPTIONS") == 0) {
		req->method = OPTIONS;
		return DLY_OK;
	}

	return HTTP_METHOD_ERROR;
}

int set_protocol_for_request(http_request_t *req)
{
	char pro[10], *p;
	int i = 0, major = 0, minor = 0;
	p = req->protocol_start;
	while(*p != '/' && p <= req->protocol_end) {
		if(*p == ' ') {
			p++;
			continue;
		}
		pro[i] = *p;
		i++;
		p++;
	}
	pro[i] = '\0';
	p++;
	if(strcmp(pro, "HTTP") != 0) {
		return HTTP_PROTOCOL_ERROR;
	}
	while(*p != '.' && p <= req->protocol_end) {
		if(*p < '0' || *p > '9') {
			return HTTP_PROTOCOL_ERROR;
		}
		major = (*p - '0') + major * 10;
		p++;
	}
	p++;
	while(p <= req->protocol_end) {
		if(*p < '0' || *p > '9') {
			return HTTP_PROTOCOL_ERROR;
		}
		minor = (*p - '0') + minor * 10;
		p++;
	}
	req->http_major = major;
	req->http_minor = minor;
	return DLY_OK;
}

int set_url_for_request(http_request_t *req)
{
	char *p;
	p = req->url_start;
	while(*p == ' ') {
		p++;
	}
	req->path_start = p;
	while(*p != '?' && p <= req->url_end) {
		p++;
	}
	req->path_end = p - 1;
	if(p == req->url_end) {
		req->query_start = req->query_end = p;
		return DLY_OK;
	}
	req->query_start = p + 1;
	req->query_end = req->url_end - 1;
	return DLY_OK;
}