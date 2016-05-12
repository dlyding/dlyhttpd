#include "http.h"

void dorequest(struct schedule *s, void *ud)
{
	http_request_t *req = (http_request_t *)ud;
    int fd = req->fd;
    int rc;
    char *filename;
    char *querystring;
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
            goto err;
        }

        if (n < 0) {
            if (errno != EAGAIN) {
                log_err("read err, and errno = %d", errno);
                goto err;
            }
            break;
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
            goto err;
        }

        rc = set_method_for_request(req);
        rc = set_protocol_for_request(req);
        rc = set_url_for_request(req);

        

        log_info("method == %.*s",req->method_end - req->request_start, req->request_start);
        log_info("uri == %.*s", req->uri_end - req->uri_start, req->uri_start);

        log_info("ready to parse request body");
        rc  = zv_http_parse_request_body(r);
        if (rc == DLY_EAGAIN) {
            continue;
        }
        else if (rc != DLY_OK) {
            log_err("rc != DLY_OK");
            goto err;
        }
        
        /*
        *   handle http header
        */
        zv_http_out_t *out = (zv_http_out_t *)malloc(sizeof(zv_http_out_t));
        if (out == NULL) {
            log_err("no enough space for zv_http_out_t");
            exit(1);
        }

        rc = zv_init_out_t(out, fd);
        check(rc == ZV_OK, "zv_init_out_t");

        parse_uri(r->uri_start, r->uri_end - r->uri_start, filename, NULL);

        if(stat(filename, &sbuf) < 0) {
            do_error(fd, filename, "404", "Not Found", "zaver can't find the file");
            continue;
        }

        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            do_error(fd, filename, "403", "Forbidden",
                    "zaver can't read the file");
            continue;
        }
        
        out->mtime = sbuf.st_mtime;

        zx_http_handle_header(r, out);
        check(list_empty(&(r->list)) == 1, "header list should be empty");
        
        if (out->status == 0) {
            out->status = ZV_HTTP_OK;
        }

        serve_static(fd, filename, sbuf.st_size, out);

        free(out);
        if (!out->keep_alive) {
            log_info("no keep_alive! ready to close");
            goto close;
        }

    }
    
    return;

err:
close:
    //free(ptr);
    close(fd);
}