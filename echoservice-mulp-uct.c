#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include "coroutine.h"

int make_socket_non_blocking(int fd);
void acceptfun(struct schedule *s, void *ud);
void dorequest(struct schedule *s, void *ud);
void workerloop(int listenfd);

int main()
{
	int listenfd, optval = 1;
	int i;
	int port = 9503;
	struct sockaddr_in serveraddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

	bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port);

    bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    //printf("%d\n", errno);

    listen(listenfd, 1024);
    //printf("%d\n", errno);

    make_socket_non_blocking(listenfd);
    //printf("%d\n", errno);

    printf("start..\n"); 

    pid_t pid[4] = {-1, -1, -1, -1};
    for(i = 0; i < 4; i++) {
        pid[i] = fork();
        if(0 == pid[i] || -1 == pid[i]) 
            break;
    }
    if(0 == pid[0]) {
        workerloop(listenfd);
    }
    if(0 == pid[1]) {
        workerloop(listenfd);
    }
    if(0 == pid[2]) {
        workerloop(listenfd);
    }
    if(0 == pid[3]) {
        workerloop(listenfd);
    }
    if(pid[0] != 0 && pid[1] != 0 && pid[2] != 0 && pid[3] != 0) {
        for(i = 0; i < 4; i++) {
            waitpid(pid[i], NULL, 0);
        }
        close(listenfd);
    }
    return 0;
}

void workerloop(int listenfd)
{
    int i;
    struct schedule * S = coroutine_open();
    printf("start..\n");
    int co1 = coroutine_new(S, acceptfun, (void *)&listenfd);
    printf("%d\n", S->cap);
    printf("%d\n", co1);
    for(i = 0; i < S->cap; i++) {
        if(coroutine_status(S, i)) {
            coroutine_resume(S,i);
        }
        if(i == S->cap - 1)
            i = -1;
    }
    coroutine_close(S);
    close(listenfd);
}

void acceptfun(struct schedule *s, void *ud)
{
    int *lfd = (int *)ud;
    int *pcfd;
    struct sockaddr_in clientaddr; 
    memset(&clientaddr, 0, sizeof(struct sockaddr_in)); 
    socklen_t len = sizeof(clientaddr);
    while(1){
        //printf("start accept..\n");
        pcfd = (int *)malloc(sizeof(int));
        *pcfd = accept(*lfd, (struct sockaddr *)&clientaddr, &len);
        //printf("%d\n", *pcfd);
        if(*pcfd == -1){
            free(pcfd);
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                coroutine_yield(s);
            }
            else {
                coroutine_yield(s);
            }
        }
        else {
            printf("%d\n", getpid());
            make_socket_non_blocking(*pcfd);
            int co = coroutine_new(s, dorequest, (void *)pcfd);
        }
    }
}

void dorequest(struct schedule *s, void *ud)
{
    int *pfd = (int *)ud;
    char in[1024];
    char *out = "hello\n";
    while(1) {
        int n = read(*pfd, in, 1023);
        if(n == 0) {
            close(*pfd);
            free(pfd);
            return;
        }
        else if(n < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                coroutine_yield(s);
            }
            else
                break;
        }
        else {
            printf("receive: %s", in);
            write(*pfd, out, 6);
            usleep(10000);
            break;
        }
    }
    close(*pfd);
    free(pfd);
    return; 
}

int make_socket_non_blocking(int fd) 
{
    int flags, s;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
        return -1;
    }

    return 0;
}
