#include "http_parse.h"

int http_parse_request_line(http_request_t *req) {
    char ch, *p;
    enum {
        st_line_start = 0,
        st_method,
        st_url,
        st_protocol,
        st_space_end,
        st_CR_end
    } state;

    state = req->state;

    // log_info("ready to parese request line, start = %d, last= %d", (int)r->pos, (int)r->last);
    for (p = req->pos; p < req->last; p++) {
        ch = *p;

        switch (state) {

        /* HTTP methods: GET, HEAD, POST */
        case st_line_start:
            if (ch == CR || ch == LF) {
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_') {
                return HTTP_METHOD_ERROR;
            }

            req->method_start = req->request_start = p;
            state = st_method;
            break;

        case st_method:
            if (ch == ' ') {
                req->url_start = req->method_end = p;
                state = st_url;
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_') {
                return HTTP_METHOD_ERROR;
            }

            break;

        case st_url:

            switch (ch) {
            case ' ':
                req->protocol_start = req->url_end = p;
                state = st_protocol;
                break;
            default:
                break;
            }
            break;

        /* space+ after URI */
        case st_protocol:
            switch (ch) {
            case ' ':
            	req->protocol_end = p;
            	state = st_space_end;
                break;
            case CR:
            	req->protocol_end = p;
                state = st_CR_end;
                break;
            default:
                break;
            }
            break;

        case st_space_end:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = st_CR_end;
                break;
            default:
                return HTTP_REQUEST_ERROR;
            }
            break;

        case st_CR_end:
            req->request_end = p - 1;
            switch (ch) {
            case LF:
                goto done;
            default:
                return HTTP_REQUEST_ERROR;
            }
            break;
        default:
            return 0;
        }
    }

    req->pos = p;
    req->state = state;

    return DLY_EAGAIN;

done:

    req->pos = p + 1;

    if (req->request_end == NULL) {
        req->request_end = p;
    }

    req->state = 6;

    return DLY_OK;
}

int http_parse_request_header(http_request_t *req) {
    char c, ch, *p, *m;
    enum {
        st_head_start = 6,
        st_key,
        st_spaces_before_colon,
        st_spaces_after_colon,
        st_value,
        st_cr,
        st_crlf,
        st_crlfcr
    } state;

    state = req->state;
    //check(state == 0, "state should be 0");

    //log_info("ready to parese request body, start = %d, last= %d", r->pos, r->last);

    http_header_t *hd; 
    for (p = req->pos; p < req->last; p++) {
        ch = *p;

        switch (state) {
        case st_head_start:
            if (ch == CR || ch == LF) {
                break;
            }

            req->cur_header_key_start = p;
            state = st_key;
            break;
        case st_key:
            if (ch == ' ') {
                req->cur_header_key_end = p;
                state = st_spaces_before_colon;
                break;
            }

            if (ch == ':') {
                req->cur_header_key_end = p;
                state = st_spaces_after_colon;
                break;
            }

            break;
        case st_spaces_before_colon:
            if (ch == ' ') {
                break;
            } else if (ch == ':') {
                state = st_spaces_after_colon;
                break;
            } else {
                return HTTP_HEAD_ERROR;
            }
        case st_spaces_after_colon:
            if (ch == ' ') {
                break;
            }

            state = st_value;
            req->cur_header_value_start = p;
            break;
        case st_value:
            if (ch == CR) {
                req->cur_header_value_end = p;
                state = st_cr;
            }

            if (ch == LF) {
                req->cur_header_value_end = p;
                state = st_crlf;
            }
            
            break;
        case st_cr:
            if (ch == LF) {
                state = st_crlf;
                // save the current http header
                hd = (http_header_t *)malloc(sizeof(http_header_t));
                hd->key_start   = req->cur_header_key_start;
                hd->key_end     = req->cur_header_key_end;
                hd->value_start = req->cur_header_value_start;
                hd->value_end   = req->cur_header_value_end;

                list_add(&(hd->list), &(req->list));

                break;
            } else {
                return HTTP_HEAD_ERROR;
            }

        case st_crlf:
            if (ch == CR) {
                state = st_crlfcr;
            } else {
                req->cur_header_key_start = p;
                state = st_key;
            }
            break;

        case st_crlfcr:
            switch (ch) {
            case LF:
                goto done;
            default:
                return HTTP_HEAD_ERROR;
            }
            break;
        }   
    }

    req->pos = p;
    req->state = state;

    return DLY_EAGAIN;

done:
    req->pos = p + 1;

    req->state = st_head_start;

    return DLY_OK;
}
