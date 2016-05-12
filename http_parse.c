#include "http_parse.h"

int http_parse_request_line(http_request_t *req) {
    u_char ch, *p;
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
            r->request_end = p - 1;
            switch (ch) {
            case LF:
                goto done;
            default:
                return HTTP_REQUEST_ERROR;
            }
        }
    }

    req->pos = p;
    req->state = state;

    return DLY_EAGAIN;

done:

    req->pos = p + 1;

    if (req->request_end == NULL) {
        r->request_end = p;
    }

    req->state = st_head_start;

    return DLY_OK;
}

int http_parse_request_head(http_request_t *req) {
    u_char c, ch, *p, *m;
    enum {
        sw_head_start = 0,
        sw_key,
        sw_spaces_before_colon,
        sw_spaces_after_colon,
        sw_value,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    } state;

    state = r->state;
    check(state == 0, "state should be 0");

    //log_info("ready to parese request body, start = %d, last= %d", r->pos, r->last);

    zv_http_header_t *hd; 
    for (p = r->pos; p < r->last; p++) {
        ch = *p;

        switch (state) {
        case sw_start:
            if (ch == CR || ch == LF) {
                break;
            }

            r->cur_header_key_start = p;
            state = sw_key;
            break;
        case sw_key:
            if (ch == ' ') {
                r->cur_header_key_end = p;
                state = sw_spaces_before_colon;
                break;
            }

            if (ch == ':') {
                r->cur_header_key_end = p;
                state = sw_spaces_after_colon;
                break;
            }

            break;
        case sw_spaces_before_colon:
            if (ch == ' ') {
                break;
            } else if (ch == ':') {
                state = sw_spaces_after_colon;
                break;
            } else {
                return ZV_HTTP_PARSE_INVALID_HEADER;
            }
        case sw_spaces_after_colon:
            if (ch == ' ') {
                break;
            }

            state = sw_value;
            r->cur_header_value_start = p;
            break;
        case sw_value:
            if (ch == CR) {
                r->cur_header_value_end = p;
                state = sw_cr;
            }

            if (ch == LF) {
                r->cur_header_value_end = p;
                state = sw_crlf;
            }
            
            break;
        case sw_cr:
            if (ch == LF) {
                state = sw_crlf;
                // save the current http header
                hd = (zv_http_request_t *)malloc(sizeof(zv_http_request_t));
                hd->key_start   = r->cur_header_key_start;
                hd->key_end     = r->cur_header_key_end;
                hd->value_start = r->cur_header_value_start;
                hd->value_end   = r->cur_header_value_end;

                list_add(&(hd->list), &(r->list));

                break;
            } else {
                return ZV_HTTP_PARSE_INVALID_HEADER;
            }

        case sw_crlf:
            if (ch == CR) {
                state = sw_crlfcr;
            } else {
                r->cur_header_key_start = p;
                state = sw_key;
            }
            break;

        case sw_crlfcr:
            switch (ch) {
            case LF:
                goto done;
            default:
                return ZV_HTTP_PARSE_INVALID_HEADER;
            }
            break;
        }   
    }

    r->pos = p;
    r->state = state;

    return ZV_AGAIN;

done:
    r->pos = p + 1;

    r->state = sw_start;

    return ZV_OK;
}
