#include "util.h"
#include "http.h"

#define REBOOT1 10
#define REBOOT2 12

#ifdef _FILELOCK
#include "filelock.h"
#endif

#ifdef _FILELOCK
#define MAX_SERVE_FD_NUM    512
#endif

static const struct option long_options[]=
{
    {"help",no_argument,NULL,'?'},
    {"version",no_argument,NULL,'v'},
    {"conf",required_argument,NULL,'c'},
    {NULL,0,NULL,0}
};

static void usage() {
   fprintf(stderr,
	"dlyhttpd [option]... \n"
	"  -c|--conf <config file>  Specify config file. Default ./dlyhttpd.conf.\n"
	"  -?|-h|--help             This information.\n"
	"  -v|--version             Display program version.\n"
	);
}

schedule_t * S;
epoll_t* et;
pid_t* pid;
int isexit = 0;

#ifdef _FILELOCK
filelock_mutex_t* fmt;
int serve_fd_num = 0;           // 每个进程服务的连接数
int isachieve_lock = 0;         // 标志进程是否获得锁
int isadd_listenfd = 0;         // 是否添加listenfd
#endif

//void timeout_handler(int signo);
void reboot1_handler(int signo);
void reboot2_handler(int signo);
void changestate_handler(int signo);
void acceptfun(schedule_t *s, void *ud);
void workerloop(int listenfd);

int main(int argc, char* argv[])
{
	int rc;
    int opt = 0;
    int options_index = 0;
    char *conf_file = CONF;

    /*if (argc == 1) {
        usage();
        return 0;
    }*/
    if(argc > 1) {
    	while ((opt = getopt_long(argc, argv, "vc:?h", long_options, &options_index)) != EOF) {
        	switch (opt) {
            	case  0 : break;
            	case 'c':
                	conf_file = optarg;
                	break;
            	case 'v':
                	printf(PROGRAM_VERSION"\n");
                	return 0;
            	case ':':
            	case 'h':
            	case '?':
                	usage();
                	return 0;
        	}
    	}

    	debug("conf-file = %s", conf_file);

    	if (optind < argc) {
        	log_err("non-option ARGV-elements: ");
        	while (optind < argc)
            	log_err("%s ", argv[optind++]);
        	return 0;
    	}
    }

    rc = read_conf(conf_file, &cf);
    check(rc == DLY_OK, "read conf err");
    debug("root:%s", cf.root);
    debug("port:%d", cf.port);
    debug("worker_num:%d", cf.worker_num);
    //debug("detect_time_sec:%ld", cf.detect_time_sec);
    //debug("detect_time_usec:%ld", cf.detect_time_usec);

    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE,&sa,NULL)) {
        log_err("install sigal handler for SIGPIPI failed");
        return 0;
    }

    int listenfd;
    
    listenfd = open_listenfd(cf.port);
    rc = make_socket_non_blocking(listenfd);
    check(rc == 0, "make_socket_non_blocking");

    #ifdef _FILELOCK
    fmt = filelock_mutex_create();
    #endif

	int i;

    printf("start..\n"); 

    pid = malloc(cf.worker_num * sizeof(pid_t));
    for(i = 0; i < cf.worker_num; i++) {
    	pid[i] = -1;
    }
    for(i = 0; i < cf.worker_num; i++) {
        pid[i] = fork();
        if(0 == pid[i]) {
            break; 
        }          
        if(-1 == pid[i]) {
            return 0;
        }
    }
    for(i = 0; i < cf.worker_num; i++) {
    	if(0 == pid[i]) {
        	workerloop(listenfd);
            
        	return 0;
    	}
    }
    /*for(i = 0; i < cf.worker_num; i++) {
        close(listenfd);
        waitpid(pid[i], NULL, 0);
    }*/   
    while(1) {

        struct sigaction act1;
        act1.sa_handler = reboot1_handler;
        act1.sa_flags = 0;
        sigemptyset(&act1.sa_mask);
        sigaction(REBOOT1, &act1, NULL);

        struct sigaction act2;
        act2.sa_handler = reboot2_handler;
        act2.sa_flags = 0;
        sigemptyset(&act2.sa_mask);
        sigaction(REBOOT2, &act2, NULL);

        pid_t cpid = wait(NULL);
        for(i = 0; i < cf.worker_num; i++) {        
            if(pid[i] == cpid) {
                pid[i] = fork();
                if(pid[i] == 0) {
                    workerloop(listenfd);
                    return 0;
                }
                else
                    break;
            }
        }    
    }
    close(listenfd);

    free(pid);

    #ifdef _FILELOCK
    filelock_mutex_close(fmt);
    #endif

    return 0;
}

void workerloop(int listenfd)
{
    isexit = 0;

    struct sigaction act;
    act.sa_handler = changestate_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(16, &act, NULL);    // 使用信号16改变子进程状态

    et = epoll_create_new(0, 1024);
    S = coroutine_open();

    #ifdef _TIMEOUT
    timer_init();
    #endif

    log_info("%d worker start", getpid());
    int co1 = coroutine_new(S, acceptfun, (void *)&listenfd);
    debug("S->cap:%d, main coroutine:%d", S->cap, co1);

    #ifndef _FILELOCK
    epoll_add_para(et, listenfd, co1, EPOLLIN | EPOLLET);
    #endif
    //epoll_add(et, listenfd, (void*)&co1, EPOLLIN);

    debug("listenfd = %d", listenfd);

   /* struct itimerval timer;
    timer.it_interval.tv_sec = cf.detect_time_sec;
    timer.it_interval.tv_usec = cf.detect_time_usec;
    timer.it_value = timer.it_interval;
    setitimer(ITIMER_REAL, &timer, NULL);*/

    /*struct sigaction act;
    act.sa_handler = timeout_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);*/

    int i, n, rc;
    /*or(i = 0; i < S->cap; i++) {
        if(S->co[i] != NULL) {
            if(coroutine_status(S, i)) {
                coroutine_resume(S,i);
            }
        }
        if(i == S->cap - 1)
            i = -1;
    }*/
    while(1) {

        if(isexit == 1) {
            break;
        }

        #ifdef _TIMEOUT
        // 获取最近超时时间
        int time = get_timeout_node_time();
        debug("time = %d", time);
        #endif

        #ifdef _FILELOCK
        if(serve_fd_num == 0) {
            epoll_add_para(et, listenfd, co1, EPOLLIN | EPOLLET);
            isadd_listenfd = 1;
            debug("add listenfd");
        }
        else if(serve_fd_num > MAX_SERVE_FD_NUM) {

        }
        else {
            rc = filelock_mutex_wlock(fmt, LOCK_UNBLOCK);
            if(rc == 0) {
                isachieve_lock = 1;
                epoll_add_para(et, listenfd, co1, EPOLLIN | EPOLLET);
                isadd_listenfd = 1;
                debug("achieve mutex lock, add listenfd");
            }
        }
        #endif 

        #ifdef _TIMEOUT
        n = epoll_wait_new(et, time);
        #else
        n = epoll_wait_new(et, -1);
        #endif

        debug("n = %d", n);

        #ifdef _FILELOCK
        if(isachieve_lock == 1) {
            filelock_mutex_unlock(fmt);
            isachieve_lock = 0;
        }

        if(isadd_listenfd == 1) {
            epoll_del(et, listenfd);
            isadd_listenfd = 0;
        }
        #endif
        
        #ifdef _TIMEOUT
        timer_node_t* tn = handle_timeout_node();
        if(tn != NULL) {
            http_request_t* req = (http_request_t*)(tn->ud);
            coroutine_resume(S, req->coid);
        }
        #endif

        for(i = 0; i < n; i++) {
            debug("i = %d", i);
            debug("n = %d", n);
            debug("%d", et->events[i].data.fd);
            //fdtmp = et->events[i].data.fd;
            /*debug("fdtmp = %d", fdtmp);
            if(fdtmp == listenfd) {
                coroutine_resume(S, *(int *)(et->events[i].data.ptr));
            }
            else {
                coroutine_resume(S, *(int *)(et->events[i].data.ptr));
            }*/
            coroutine_resume(S, et->events[i].data.fd);
            debug("epoll_process");
        }
    }

    coroutine_close(S);
    close(listenfd);
    epoll_close_new(et);

    #ifdef _TIMEOUT
    timer_close();
    #endif

    #ifdef _FILELOCK
    filelock_mutex_close(fmt);
    #endif

    free(pid);
    log_info("%d worker exit", getpid());
}

void acceptfun(schedule_t *S, void *ud)
{
    debug("acceptfun");
    int *lfd = (int *)ud;
    int pcfd;
    struct sockaddr_in clientaddr; 
    memset(&clientaddr, 0, sizeof(struct sockaddr_in)); 
    socklen_t len = sizeof(clientaddr);
    while(1){
        //printf("start accept..\n");
        //pcfd = (int *)malloc(sizeof(int));
        pcfd = accept(*lfd, (struct sockaddr *)&clientaddr, &len);
        debug("pcfd = %d", pcfd);
        //printf("%d\n", *pcfd);
        if(pcfd == -1){
            //free(pcfd);
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                coroutine_yield(S);
            }
            else {
                log_err("acceptfun error, pid %d exit", getpid());
                exit(0);          // 如何优雅的退出 
            }
        }
        else {
            //printf("%d\n", getpid());
            make_socket_non_blocking(pcfd);
            http_request_t *request = (http_request_t *)malloc(sizeof(http_request_t));
            int co = coroutine_new(S, dorequest, (void *)request);
            init_request_t(request, pcfd, co, &cf);
            epoll_add_para(et, pcfd, co, EPOLLIN | EPOLLET);

            #ifdef _FILELOCK
            serve_fd_num++;
            #endif
        }
    }
}

/*void timeout_handler(int signo)
{
    int i;
    time_t now_time = time(NULL);
    double time_diff = 0;
    for(i = 1; i < S->cap; i++) {
        if(S->co[i] != NULL) {
            if(coroutine_status(S, i)) {
                time_diff = difftime(now_time, ((http_request_t *)((S->co[i])->ud))->mtime);
                if(time_diff > TIMEOUT_THRESHOLD) {
                    ((http_request_t *)((S->co[i])->ud))->istimeout = 1;
                }
            }
        }
    }
}*/

void reboot1_handler(int signo) {
    int i;
    for(i = 0; i < cf.worker_num; i++) {
        kill(pid[i], 9);
    }
}

void reboot2_handler(int signo) {
    int i;
    for(i = 0; i < cf.worker_num; i++) {
        kill(pid[i], 16);
    }
}

void changestate_handler(int signo) {
    isexit = 1;
}
