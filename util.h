#ifndef __UTIL_H__
#define __UTIL_H__

//#define _XOPEN_SOURCE 
//#define _GNU_SOURCE

#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <math.h>

#include "dbg.h"
#include "rio.h"
#include "coroutine.h"
#include "epoll_new.h"
#include "timer.h"

#define DLY_OK            0
#define DLY_NULL_POINTER -1
#define DLY_EAGAIN       -2
#define DLY_CONF_ERROR   -3

#define LISTENQ     1024

#define BUFLEN      1024
#define SHORTLEN     256

#define DELIM    "="
#define CR       '\r'
#define LF       '\n'
#define CRLFCRLF "\r\n\r\n"

#define CONF "dlyhttpd.conf"
#define PROGRAM_VERSION "1.0"
//#define TIMEOUT_THRESHOLD 10.0               // 超时阈值

typedef struct conf_s {
    char root[SHORTLEN];
    int port;
    int worker_num;
    //long detect_time_sec;
    //long detect_time_usec;
    char phpfpm_ip[SHORTLEN];
    unsigned short phpfpm_port;
}conf_t;

conf_t cf;

int read_conf(char *filename, conf_t *cf);    // 配置文件中不能有空格,#后为注释

int open_listenfd(int port);
int make_socket_non_blocking(int fd);

#endif