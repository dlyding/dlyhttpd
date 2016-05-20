#include "util.h"
#include "http_base.h"

#define CONF "dlyhttpd.conf"
#define PROGRAM_VERSION "0.1"

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

void acceptfun(struct schedule *s, void *ud);
void dorequest(struct schedule *s, void *ud);
void workerloop(int listenfd);

int main(int argc, char* argv[])
{
	int rc;
    int opt = 0;
    int options_index = 0;
    char *conf_file = CONF;

    if (argc == 1) {
        usage();
        return 0;
    }

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

    conf_t cf;
    rc = read_conf(conf_file, &cf);
    check(rc == DLY_OK, "read conf err");
    log_info("root:%s", cf.root);
    log_info("port:%d", cf.port);
    log_info("worker_num:%d", cf.worker_num);

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
    for(i = 0; i < cf.worker_num; i++) {
        waitpid(pid[i], NULL, 0);
    }
    close(listenfd);
    return 0;
}

void workerloop(int listenfd)
{
    int i;
    struct schedule * S = coroutine_open();
    printf("worker start\n");
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