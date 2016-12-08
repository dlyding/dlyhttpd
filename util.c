#include "util.h"

int read_conf(char *filename, conf_t *cf) {
	char conf_buf[BUFLEN]; 
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        log_err("cannot open config file: %s", filename);
        return DLY_CONF_ERROR;
    }
    char *delim_pos;

    while (fgets(conf_buf, BUFLEN, fp)) {
        if(conf_buf[0] == '#') {
            continue;
        }
        delim_pos = strstr(conf_buf, DELIM);  
        if (!delim_pos)
            return DLY_CONF_ERROR;
        
        if (conf_buf[strlen(conf_buf) - 1] == '\n') {
            conf_buf[strlen(conf_buf) - 1] = '\0';
        }

        if (strncmp("root", conf_buf, 4) == 0) {
            strcpy(cf->root, delim_pos + 1);
        }

        else if (strncmp("port", conf_buf, 4) == 0) {
            cf->port = atoi(delim_pos + 1);     
        }

        else if (strncmp("worker_num", conf_buf, 10) == 0) {
            cf->worker_num = atoi(delim_pos + 1);
        }

        /*else if (strncmp("detect_time_sec", conf_buf, 15) == 0) {
            cf->detect_time_sec = atoi(delim_pos + 1);
        }

        else if (strncmp("detect_time_usec", conf_buf, 16) == 0) {
            cf->detect_time_usec = atoi(delim_pos + 1);
        }*/

        else if (strncmp("phpfpm_ip", conf_buf, 9) == 0) {
            strcpy(cf->phpfpm_ip, delim_pos + 1);     
        }

        else if (strncmp("phpfpm_port", conf_buf, 11) == 0) {
            cf->phpfpm_port = (unsigned short)atoi(delim_pos + 1);
        }

        else {
            log_err("read config file error!");
            return 0;
        }

    }

    fclose(fp);
    return DLY_OK;
}

int open_listenfd(int port) 
{
    if (port <= 0) {
        port = 3000;
    }

    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	    return -1;
 
    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
		   (const void *)&optval , sizeof(int)) < 0)
	    return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	    return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
	    return -1;

    return listenfd;
}

/*
    make a socket non blocking. If a listen socket is a blocking socket, after it comes out from epoll and accepts the last connection, the next accpet will block, which is not what we want
*/
int make_socket_non_blocking(int fd) {
    int flags, s;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log_err("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
        log_err("fcntl");
        return -1;
    }
    return 0;
}

