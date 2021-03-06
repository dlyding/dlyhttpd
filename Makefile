ALGNAMES := coroutine fastcgi filelock http http_base http_handle http_parse rio util \
epoll_new heap timer connection_pool threadpool
			
INCFILES := list.h dbg.h $(addsuffix .h, $(ALGNAMES))
OBJFILES := dlyhttpd.o $(addsuffix .o, $(ALGNAMES))

EXEFILE  := dlyhttpd

world: $(EXEFILE)

$(EXEFILE): $(OBJFILES)
	gcc $(OBJFILES) -o $(EXEFILE) -lpthread

$(OBJFILES): %.o:%.c $(INCFILES)
	gcc -c $(filter %.c, $<) -o $@ -Wformat=0

clean:
	rm -rf $(OBJFILES) $(EXEFILE)