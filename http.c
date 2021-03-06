#include "http.h"

#ifdef _FILELOCK
extern int serve_fd_num;
#endif

static void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
static void serve_static(int fd, int methodID, char *filename, size_t filesize, http_response_t *res);
void serve_php(int sfd, int methodID, char *filename, char *querystring, http_response_t *res);
void do_options(int fd);
int cal_length_of_message(char* message, int length);

void dorequest(schedule_t *s, void *ud)
{
	http_request_t *req = (http_request_t *)ud;
    //conf_t cf;
    int fd = req->fd;
    int rc;
    char filename[SHORTLINE];
    char querystring[SHORTLINE];
    struct stat sbuf;
    int n;
    char *root = req->root;
    debug("root=%s", root);
    debug("process %d is dorequest", getpid());
    
    // 删除超时模块
    for(;;) {
        n = read(fd, req->last, req->buf + MAX_BUF - req->last);
        check(req->buf + MAX_BUF > req->last, "req->buf + MAX_BUF");

        if (n == 0) {   // EOF
            debug("read return 0, ready to close fd %d", fd);

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            goto close;
        }

        if (n < 0) {
            if (errno != EAGAIN) {
                log_err("read err, and errno = %d", errno);

                #ifdef _TIMEOUT
                if(req->timer != NULL) {
                    req->timer->istimeout = 1;
                }
                #endif

                goto close;
            }
            coroutine_yield(s);

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                if(req->timer->istimeout) {
                    debug("timeout, ready to close fd %d", fd);
                    // 超时模块中删除
                    goto close;
                }
            }
            #endif

            continue;
        }
        //req->mtime = time(NULL);
        req->last += n;
        check(req->last <= req->buf + MAX_BUF, "req->last <= MAX_BUF");
        debug("has read %d, buffer remaining: %ld, buffer rece:%s", n, req->buf + MAX_BUF - req->last, req->buf);
        
        debug("ready to parse request line"); 
        rc = http_parse_request_line(req);
        if (rc == DLY_EAGAIN) {
            continue;
        }
        else if (rc != DLY_OK) {
            debug("http_parse_request_line error!");

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif    

            goto close;
        }

        debug("ready to parse request header");
        rc = http_parse_request_header(req);
        if (rc == DLY_EAGAIN) {
            continue;
        }
        else if (rc != DLY_OK) {
            debug("http_parse_request_header error!");

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            goto close;
        }
        
        rc = set_method_for_request(req);
        check(rc == DLY_OK, "set_method_for_request error, %d", rc);
        debug("method = %d", req->method);

        rc = set_protocol_for_request(req);
        check(rc == DLY_OK, "set_protocol_for_request error, %d", rc);
        debug("HTTP version = %d.%d", req->http_major, req->http_minor);

        if(req->method == OPTIONS) {
            do_options(req->fd);

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            goto close;
        }

        rc = set_url_for_request(req);
        check(rc == DLY_OK, "set_url_for_request error, %d", rc);
        debug("uri = %.*s", req->url_end - req->url_start, req->url_start);
        // apply space for filename
        // apply space for querystring
        rc = get_information_from_url(req, filename, querystring);
        check(rc == DLY_OK, "get_information_from_url error, %d", rc);
        debug("filename = %s, querystring = %s", filename, querystring);      
        /*
        *   handle http header
        */
        http_response_t *res = (http_response_t *)malloc(sizeof(http_response_t));
        if (res == NULL) {
            log_err("no enough space for zv_http_out_t");

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            goto close;
        }

        rc = init_response_t(res, fd);
        check(rc == DLY_OK, "init_response_t error, %d", rc);

        if(stat(filename, &sbuf) < 0) {
            res->status = HTTP_NOT_FOUND;
            do_error(fd, filename, "404", "Not Found", "dlyhttpd can't find the file");
            free(res);

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            goto close;
        }

        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            res->status = HTTP_FORBIDDEN;
            do_error(fd, filename, "403", "Forbidden", "dlyhttpd can't read the file");
            free(res);

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            goto close;
        }
        
        res->mtime = sbuf.st_mtime;

        http_handle_header(req, res);
        check(list_empty(&(req->list)) == 1, "header list should be empty");
        
        char *filetype = rindex(filename, '.');
        if(strcmp(".php", filetype) == 0) {
            serve_php(fd, req->method, filename, querystring, res);
        }
        else{
            serve_static(fd, req->method, filename, sbuf.st_size, res);
        }
        
        if (!res->keep_alive) {
            debug("no keep_alive! ready to close");
            free(res);

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            goto close;
        }
        else{
            // 释放空间 ，加入超时模块
            free(res);
            http_request_t* reqnew = (http_request_t*)malloc(sizeof(http_request_t));
            init_request_t_copy(reqnew, req);

            #ifdef _TIMEOUT
            if(req->timer != NULL) {
                req->timer->istimeout = 1;
            }
            #endif

            free(req);
            req = reqnew;

            #ifdef _TIMEOUT
            req->timer = add_timer(req, TIMEOUT_DEFAULT);
            #endif

            coroutine_yield(s);
            // yield
            
            #ifdef _TIMEOUT
            if(req->timer->istimeout) {
                debug("timeout, ready to close fd %d", fd);
                // 超时模块中删除
                goto close;
            }
            #endif
        }

    }
    /*if(req->istimeout) {
        debug("timeout, ready to close fd %d", fd);
        goto close;
    }   // 超时怎么处理*/
    return;

close:
    free(req);
    close(fd);

    #ifdef _FILELOCK
    serve_fd_num--;
    #endif
}

void do_options(int fd) {
    char header[MAXLINE];
    sprintf(header, "HTTP/1.1 204 No Content\r\n");
    sprintf(header, "%sAllow: GET, HEAD, OPTIONS\r\n\r\n", header);
    rio_writen(fd, header, strlen(header));
}

void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char header[MAXLINE], body[MAXLINE];

    sprintf(body, "<html><title>dlyhttpd Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n</p>", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The dlyhttpd web server</em>\r\n", body);

    sprintf(header, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    sprintf(header, "%sServer: dlyhttpd\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));
    //debug("header  = \n %s\n", header);
    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));
    debug("leave clienterror\n");
    return;
}

void serve_static(int fd, int methodID, char *filename, size_t filesize, http_response_t *res) 
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
        sprintf(header, "%sCache-Control: %s\r\n", header, "max-age = 3");
        sprintf(header, "%sLast-Modified: %s\r\n", header, buf);
    } 
    else {
        // TODO
    }

    sprintf(header, "%sServer: dlyhttpd\r\n", header);
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

    /*int srcfd = open(filename, O_RDONLY, 0);
    check(srcfd > 2, "open error");
    // can use sendfile
    char *srcaddr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr > 0, "mmap error");
    close(srcfd);

    n = rio_writen(fd, srcaddr, filesize);
    // check(n == filesize, "rio_writen error");

    munmap(srcaddr, filesize);*/
    if(methodID == HEAD)
        goto out;
    int srcfd = open(filename, O_RDONLY, 0);
    //sendfile(fd, srcfd, NULL, 5);
    sendfile(fd, srcfd, NULL, filesize);
    close(srcfd);

out:
    return;
}

void serve_php(int sfd, int methodID, char *filename, char *querystring, http_response_t *res)
{
    int cfd;
    struct sockaddr_in serv_addr;
    int str_len;
    int contentLengthR;
    char header[MAXLINE];
    char method[METHODLEN];

    sprintf(method, "%s", get_method_string_from_methodID(methodID));
    sprintf(header, "HTTP/1.1 %d %s\r\n", res->status, get_shortmsg_from_status_code(res->status));
    sprintf(header, "%sServer: dlyhttpd\r\n", header);
    if (res->keep_alive) {
        sprintf(header, "%sConnection: keep-alive\r\n", header);
    }

    // 创建套接字
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == cfd){
        log_err("socket error!");
    }
    //printf("%u\n", cf.phpfpm_port);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(cf.phpfpm_ip);
    serv_addr.sin_port = htons(cf.phpfpm_port);

    // 连接服务器
    if(-1 == connect(cfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))){
        log_err("connetct error!");
    }


    // 首先构造一个FCGI_BeginRequestRecord结构
    FCGI_BeginRequestRecord beginRecord;
    beginRecord.header = 
        make_header(FCGI_BEGIN_REQUEST, FCGI_REQUEST_ID, sizeof(beginRecord.body), 0);
    beginRecord.body = make_beginRequestBody(FCGI_RESPONDER);

    str_len = write(cfd, &beginRecord, sizeof(beginRecord));
    if(-1 == str_len){
        log_err("Write beginRecord failed!");
    }

    // 传递FCGI_PARAMS参数
    char *params[][2] = {
        {"SCRIPT_FILENAME", filename}, 
        {"REQUEST_METHOD", method}, 
        {"QUERY_STRING", querystring}, 
        {"", ""}
    };

    int i, contentLength, paddingLength;
    FCGI_ParamsRecord *paramsRecordp;
    for(i = 0; params[i][0] != ""; i++){
        contentLength = strlen(params[i][0]) + strlen(params[i][1]) + 2; // 2字节是存放名-值长度的两字节
        paddingLength = (contentLength % 8) == 0 ? 0 : 8 - (contentLength % 8);
        paramsRecordp = (FCGI_ParamsRecord *)malloc(sizeof(FCGI_ParamsRecord) + contentLength + paddingLength);
        paramsRecordp->nameLength = (unsigned char)strlen(params[i][0]);    // 填充参数值
        paramsRecordp->valueLength = (unsigned char)strlen(params[i][1]);   // 填充参数名
        paramsRecordp->header = 
            make_header(FCGI_PARAMS, FCGI_REQUEST_ID, contentLength, paddingLength);
        memset(paramsRecordp->data, 0, contentLength + paddingLength);
        memcpy(paramsRecordp->data, params[i][0], strlen(params[i][0]));
        memcpy(paramsRecordp->data + strlen(params[i][0]), params[i][1], strlen(params[i][1]));
        str_len = write(cfd, paramsRecordp, 8 + contentLength + paddingLength);

        if(-1 == str_len){
            //errorHandling("Write beginRecord failed!");
        }
        //printf("Write params %s  %s\n",params[i][0], params[i][1]);
        free(paramsRecordp);

    }

    // 传递FCGI_STDIN参数
    FCGI_Header stdinHeader;
    stdinHeader = make_header(FCGI_STDIN, FCGI_REQUEST_ID, 0, 0);
    write(cfd, &stdinHeader, sizeof(stdinHeader));

    // 读取解析FASTCGI应用响应的数据
    FCGI_Header respHeader;
    char *message;
    str_len = read(cfd, &respHeader, 8);
    if(-1 == str_len){
        //errorHandling("read responder failed!");
    }
    //printf("Start read....\n");
    //printf("fastcgi responder is : %X\n", respHeader.type);
    //printf("fastcgi responder is : %X\n", respHeader.contentLengthB1);
    //printf("fastcgi responder is : %X\n", respHeader.contentLengthB0);
    if(respHeader.type == FCGI_STDOUT){
        contentLengthR = 
            ((int)respHeader.contentLengthB1 << 8) + (int)respHeader.contentLengthB0;
        //printf("conteng length is : %d\n", (int)respHeader.contentLengthB1 << 8);
        //printf("conteng length is : %d\n", (int)respHeader.contentLengthB0);
        //printf("conteng length is : %d\n", contentLengthR);
        message = (char *)malloc(contentLengthR);
        read(cfd, message, contentLengthR);
        printf("%s\n",message);
    }
    sprintf(header, "%sContent-length: %d\r\n", header, cal_length_of_message(message, contentLengthR));
    printf("%s",header);
    printf("%s\n",message);
    write(sfd, header, strlen(header)); 
    write(sfd, message, contentLengthR);

    free(message);
    close(cfd);
}

int cal_length_of_message(char* message, int length) {
    int count = 0;
    int i = 0;
    while(i < length) {
        if(message[i] == '\r' && message[i + 2] == '\r') {
            count = length - i - 4;
            break;
        }
        else {
            i++;
        }       
    }
    return count;
}
