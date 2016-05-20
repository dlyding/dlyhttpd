#ifndef __UTIL_H__
#define __UTIL_H__

#include <sys/socket.h>
#include <sys/types.h>
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

#include "dbg.h"
#include "rio.h"
#include "coroutine.h"

#define DLY_OK            0
#define DLY_NULL_POINTER -1
#define DLY_EAGAIN       -2
#define DLY_CONF_ERROR   100

#define LISTENQ     1024

#define BUFLEN      1024
#define ROOTLEN     256

#define DELIM    "="
#define CR       '\r'
#define LF       '\n'
#define CRLFCRLF "\r\n\r\n"

typedef struct conf_s {
    char root[ROOTLEN];
    int port;
    int worker_num;
}conf_t;

int read_conf(char *filename, conf_t *cf);    // 配置文件中不能有空格

int open_listenfd(int port);
int make_socket_non_blocking(int fd);

#endif