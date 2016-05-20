#include "http.h"

static void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
static void serve_static(int fd, char *filename, size_t filesize, http_response_t *res);
//static void dorequest_line(struct schedule *s, http_request_t *req, char *filename, char *querystring);
//static void dorequest_header(struct schedule *s, http_request_t *req);

void dorequest(struct schedule *s, void *ud)
{
	http_request_t *req = (http_request_t *)ud;
    int fd = req->fd;
    int rc;
    char filename[SHORTLINE];
    char querystring[SHORTLINE];
    struct stat sbuf;
    int n;
    char *root = req->root;
    debug("root=%s", root);
    
    for(;;) {
        n = read(fd, req->last, (uint64_t)req->buf + MAX_BUF - (uint64_t)req->last);
        check((uint64_t)req->buf + MAX_BUF > (uint64_t)req->last, "(uint64_t)req->buf + MAX_BUF");
        //log_info("has read %d, buffer remaining: %d, buffer rece:%s", n, (uint64_t)r->buf + MAX_BUF - (uint64_t)r->last, r->buf);

        if (n == 0) {   // EOF
            log_info("read return 0, ready to close fd %d", fd);
            goto err2;
        }

        if (n < 0) {
            if (errno != EAGAIN) {
                log_err("read err, and errno = %d", errno);
                goto err2;
            }
            coroutine_yield(s);
            continue;
        }

        req->last += n;
        check(req->last <= req->buf + MAX_BUF, "req->last <= MAX_BUF");
        
        log_info("ready to parse request line"); 
        rc = http_parse_request_line(req);
        if (rc == DLY_EAGAIN) {
            continue;
        }
        else if (rc != DLY_OK) {
            log_err("rc != DLY_OK");
            goto err2;
        }

        log_info("ready to parse request header");
        rc = http_parse_request_header(req);
        if (rc == DLY_EAGAIN) {
            continue;
        }
        else if (rc != DLY_OK) {
            log_err("rc != DLY_OK");
            goto err2;
        }

        rc = set_method_for_request(req);
        rc = set_protocol_for_request(req);
        rc = set_url_for_request(req);

        // apply space for filename
        // apply space for querystring

        rc = get_information_from_url(req, filename, querystring);        

        log_info("method == %.*s",req->method_end - req->request_start, req->request_start);
        log_info("uri == %.*s", req->uri_end - req->uri_start, req->uri_start);     
        /*
        *   handle http header
        */
        http_response_t *res = (http_response_t *)malloc(sizeof(http_response_t));
        if (res == NULL) {
            log_err("no enough space for zv_http_out_t");
            goto err2;
        }

        rc = init_response_t(res, fd);
        check(rc == DLY_OK, "init_response_t");

        if(stat(filename, &sbuf) < 0) {
            do_error(fd, filename, "404", "Not Found", "dlyhttpd can't find the file");
            goto err1;
        }

        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            do_error(fd, filename, "403", "Forbidden", "dlyhttpd can't read the file");
            goto err1;
        }
        
        res->mtime = sbuf.st_mtime;

        http_handle_header(req, res);
        check(list_empty(&(req->list)) == 1, "header list should be empty");
        
        if (res->status == 0) {
            res->status = ZV_HTTP_OK;
        }
        char *filetype = rindex(filename, '.');
        if(strcmp(".php", filetype) == 0) {
            // TODO
        }
        else{
            serve_static(fd, filename, sbuf.st_size, res);
        }
        
        if (!res->keep_alive) {
            log_info("no keep_alive! ready to close");
            free(res);
            goto close;
        }
        else{
            // 释放空间
            free(res);
            init_request_t(req);
        }

    }
    
    return;

err1:
    free(res);
err2:
close:
    free(req);
    close(fd);
}

void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char header[MAXLINE], body[MAXLINE];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n</p>", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny web server</em>\r\n", body);

    sprintf(header, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    sprintf(header, "%sServer: dlyhttpd\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));
    //log_info("header  = \n %s\n", header);
    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));
    log_info("leave clienterror\n");
    return;
}

void serve_static(int fd, char *filename, size_t filesize, http_response_t *res) 
{
    char header[MAXLINE];
    char buf[SHORTLINE];
    int n;
    struct tm tm;
    
    const char *file_type;
    const char *dot_pos = rindex(filename, '.');
    file_type = get_file_type(dot_pos);

    sprintf(header, "HTTP/1.1 %d %s\r\n", res->status, get_shortmsg_from_status_code(res->status));

    if (res->keep_alive) {
        sprintf(header, "%sConnection: keep-alive\r\n", header);
    }

    if (res->modified) {
        sprintf(header, "%sContent-type: %s\r\n", header, file_type);
        sprintf(header, "%sContent-length: %zu\r\n", header, filesize);
        localtime_r(&(res->mtime), &tm);
        strftime(buf, SHORTLINE,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sLast-Modified: %s\r\n", header, buf);
    } 
    else {
        // TODO
    }

    sprintf(header, "%sServer: Zaver\r\n", header);
    sprintf(header, "%s\r\n", header);

    n = rio_writen(fd, header, strlen(header));
    check(n == strlen(header), "rio_writen error, errno = %d", errno);
    if (n != strlen(header)) {
        log_err("n != strlen(header)");
        goto out; 
    }

    if (!res->modified) {
        goto out;
    }

    int srcfd = open(filename, O_RDONLY, 0);
    check(srcfd > 2, "open error");
    // can use sendfile
    char *srcaddr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr > 0, "mmap error");
    close(srcfd);

    n = rio_writen(fd, srcaddr, filesize);
    // check(n == filesize, "rio_writen error");

    munmap(srcaddr, filesize);

out:
    return;
}