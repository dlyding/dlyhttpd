#ifndef __CONNECTION_POOL__
#define __CONNECTION_POOL__

#define MINCONNECTION 10
#define MAXCONNECTION 30

typedef struct connection_pool_s {
	int minConnection;
	int maxConnection;
	int curConnection;
	int usedConnection;
	char *ip;
	int port;
	struct connection_s **cons;
} connection_pool_t;

typedef struct connection_s {
	int fd;
	int isused;
} connection_t;

connection_pool_t* connection_pool_create(char* ip, int port);
connection_t* get_connection(connection_pool_t* cpt);
void put_connection(connection_pool_t* cpt, connection_t* ct);
void connection_pool_close(connection_pool_t* cpt);

#endif
