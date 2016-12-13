#include "util.h"
#include "http.h"

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

void timeout_handler(int signo);
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
    log_info("root:%s", cf.root);
    log_info("port:%d", cf.port);
    log_info("worker_num:%d", cf.worker_num);
    //log_info("detect_time_sec:%ld", cf.detect_time_sec);
    //log_info("detect_time_usec:%ld", cf.detect_time_usec);

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

	int i;

    printf("start..\n"); 

    pid_t *pid = malloc(cf.worker_num * sizeof(pid_t));
    for(i = 0; i < cf.worker_num; i++) {
    	pid[i] = -1;
    }
    for(i = 0; i < cf.worker_num; i++) {
        pid[i] = fork();
        if(0 == pid[i]) 
            break;
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
    return 0;
}

void workerloop(int listenfd)
{
    et = epoll_create_new(0, 1024);
    S = coroutine_open();
    timer_init();
    log_info("%d worker start", getpid());
    int co1 = coroutine_new(S, acceptfun, (void *)&listenfd);
    log_info("S->cap:%d, main coroutine:%d", S->cap, co1);

    epoll_add_para(et, listenfd, co1, EPOLLIN | EPOLLET);
    //epoll_add(et, listenfd, (void*)&co1, EPOLLIN);

    log_info("listenfd = %d", listenfd);

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

    int i, n;
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
        // 获取最近超时时间
        int time = get_timeout_node_time();
        log_info("time = %d", time);
        n = epoll_wait_new(et, time);
        log_info("n = %d", n);
        // 如果是超时，怎么办
        timer_node_t* tn = handle_timeout_node();
        if(tn != NULL) {
            http_request_t* req = (http_request_t*)(tn->ud);
            coroutine_resume(S, req->coid);
        }
        for(i = 0; i < n; i++) {
            log_info("i = %d", i);
            log_info("n = %d", n);
            log_info("%d", et->events[i].data.fd);
            //fdtmp = et->events[i].data.fd;
            /*log_info("fdtmp = %d", fdtmp);
            if(fdtmp == listenfd) {
                coroutine_resume(S, *(int *)(et->events[i].data.ptr));
            }
            else {
                coroutine_resume(S, *(int *)(et->events[i].data.ptr));
            }*/
            coroutine_resume(S, et->events[i].data.fd);
            log_info("epoll_process");
        }
    }

    coroutine_close(S);
    close(listenfd);
    epoll_close_new(et);
    timer_close();
}

void acceptfun(schedule_t *S, void *ud)
{
    log_info("acceptfun");
    int *lfd = (int *)ud;
    int pcfd;
    struct sockaddr_in clientaddr; 
    memset(&clientaddr, 0, sizeof(struct sockaddr_in)); 
    socklen_t len = sizeof(clientaddr);
    while(1){
        //printf("start accept..\n");
        //pcfd = (int *)malloc(sizeof(int));
        pcfd = accept(*lfd, (struct sockaddr *)&clientaddr, &len);
        log_info("pcfd = %d", pcfd);
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
