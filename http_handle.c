#include "http_handle.h"

mime_type_t http_mime[] = 
{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".bmp", "image/bmp"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {".js", "application/x-javascript"},
    {NULL ,"text/plain"}
};

static void http_process_ignore(http_request_t *req, http_response_t *res, char *data, int len);
static void http_process_connection(http_request_t *req, http_response_t *res, char *data, int len);
static void http_process_if_modified_since(http_request_t *req, http_response_t *res, char *data, int len);
static void http_process_content_length(http_request_t *req, http_response_t *res, char *data, int len);

http_header_handle_t http_headers_in[] = {
    {"Host", http_process_ignore},
    {"Connection", http_process_connection},
    {"If-Modified-Since", http_process_ignore},
    {"Content-Length", http_process_content_length},
    {"", http_process_ignore}
};

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
	while(*p != '/' && p < req->protocol_end) {
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
	while(*p != '.' && p < req->protocol_end) {
		if(*p == ' ') {
			p++;
			continue;
		}
		if(*p < '0' || *p > '9') {
			return HTTP_PROTOCOL_ERROR;
		}
		major = (*p - '0') + major * 10;		
		p++;
	}
	p++;
	while(p < req->protocol_end) {
		if(*p == ' ') {
			p++;
			continue;
		}
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
	while(*p != '?' && p < req->url_end) {
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

const char* get_file_type(const char *type)
{
    if (type == NULL) {
        return "text/plain";
    }

    int i;
    for (i = 0; http_mime[i].type != NULL; ++i) {
        if (strcmp(type, http_mime[i].type) == 0)
            return http_mime[i].value;
    }
    return http_mime[i].value;
}

int get_information_from_url(const http_request_t *req, char *filename, char *querystring)
{
	strcpy(filename, req->root);
	if(req->path_start == req->path_end) {
		if(*(req->path_start) == '/') {
			strcat(filename, "/index.html");
		}
		else {
			return HTTP_PATH_ERROR;
		}
	}
	else {
		strncat(filename, req->path_start, req->path_end - req->path_start + 1);
	}
	if(querystring == NULL) {
		return DLY_OK;
	}
	if(req->query_end <= req->query_start) {
		strcpy(querystring, "");
		return DLY_OK;
	}
	else {
		strncpy(querystring, req->query_start, req->query_end - req->query_start + 1);
		return DLY_OK;
	}
}

void http_handle_header(http_request_t *req, http_response_t *res) {
    list_head *pos;
    http_header_t *hd;
    http_header_handle_t *header_in;

    list_for_each(pos, &(req->list)) {
        hd = list_entry(pos, http_header_t, list);
        debug("key = %.*s, value = %.*s", hd->key_end - hd->key_start, hd->key_start, hd->value_end - hd->value_start, hd->value_start);
        /* handle */

        for (header_in = http_headers_in; strlen(header_in->name) > 0; header_in++) {
            if (strncmp(hd->key_start, header_in->name, hd->key_end - hd->key_start) == 0) {         
                int len = hd->value_end - hd->value_start;
                (*(header_in->handler))(req, res, hd->value_start, len);
                break;
            }    
        }

        /* delete it from the original list */
        list_del(pos);
        free(hd);
    }
}

static void http_process_ignore(http_request_t *req, http_response_t *res, char *data, int len) {
    
}

static void http_process_connection(http_request_t *req, http_response_t *res, char *data, int len) {
    if (strncasecmp("keep-alive", data, len) == 0) {
        res->keep_alive = 1;
    }

}

static void http_process_if_modified_since(http_request_t *req, http_response_t *res, char *data, int len) {
    struct tm tm;
    strptime(data, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    time_t client_time = mktime(&tm);

    double time_diff = difftime(res->mtime, client_time);
    if (fabs(time_diff) < 1e6) {
        debug("not modified!!");
        /* Not modified */
        res->modified = 0;
        res->status = HTTP_NOT_MODIFIED;
    }
}

static void http_process_content_length(http_request_t *req, http_response_t *res, char *data, int len)
{
	char *temp = (char *)malloc(len + 1);
	strncpy(temp, data, len);
	req->body_length = atoi(temp);
}

const char *get_shortmsg_from_status_code(int status_code) {
    /*  for code to msg mapping, please check: 
    * http://users.polytech.unice.fr/~buffa/cours/internet/POLYS/servlets/Servlet-Tutorial-Response-Status-Line.html
    */
    if (status_code = HTTP_OK) {
        return "OK";
    }

    if (status_code = HTTP_NOT_MODIFIED) {
        return "Not Modified";
    }

    if (status_code = HTTP_FORBIDDEN) {
        return "Forbidden";
    }

    if (status_code = HTTP_NOT_FOUND) {
        return "Not Found";
    }
    
    return "Unknown";
}