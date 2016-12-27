#include "connection_pool.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

connection_pool_t* connection_pool_create(char* ip, int port) {
	connection_pool_t* cpt = (connection_pool_t*)malloc(sizeof(connection_pool_t));
	cpt->minConnection = MINCONNECTION;
	cpt->maxConnection = MAXCONNECTION;
	cpt->curConnection = 0;
	cpt->usedConnection = 0;
	cpt->ip = (char*)malloc(strlen(ip) + 1);
	strcpy(cpt->ip, ip);
	cpt->port = port;
	cpt->cons = (struct connection_s**)malloc(sizeof(connection_t*) * cpt->maxConnection);
	memset(cpt->cons, 0, sizeof(connection_t*) * cpt->maxConnection);

	struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(cpt->ip);
    serv_addr.sin_port = htons(cpt->port);
	for(int i = 0; i < cpt->minConnection; i++) {
		connection_t* ct = (connection_t*)malloc(sizeof(connection_t));
		ct->fd = socket(AF_INET, SOCK_STREAM, 0);
		connect(ct->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
		ct->isused = 0;
		cpt->cons[i] = ct;
		cpt->curConnection++;
	}
	return cpt;
}

connection_t* get_connection(connection_pool_t* cpt) {
	if(cpt->usedConnection < cpt->curConnection) {
		int i;
		for(i = 0; i < cpt->curConnection; i++) {
			int id = (cpt->usedConnection + i) % cpt->curConnection;
			if(cpt->cons[id]->isused == 0) {
				cpt->usedConnection++;
				cpt->cons[id]->isused = 1;
				return cpt->cons[id];
			}				
		}
	}
	else {
		if(cpt->curConnection < cpt->maxConnection) {
			struct sockaddr_in serv_addr;
    		memset(&serv_addr, 0, sizeof(serv_addr));
    		serv_addr.sin_family = AF_INET;
    		serv_addr.sin_addr.s_addr = inet_addr(cpt->ip);
    		serv_addr.sin_port = htons(cpt->port);
			connection_t* ct = (connection_t*)malloc(sizeof(connection_t));
			ct->fd = socket(AF_INET, SOCK_STREAM, 0);
			connect(ct->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
			ct->isused = 1;
			cpt->cons[cpt->curConnection] = ct;
			cpt->curConnection++;
			cpt->usedConnection++;
			return ct;
		}
		else {
			return NULL;
		}
	}
	return NULL;
}

void put_connection(connection_pool_t* cpt, connection_t* ct) {
	ct->isused = 0;
}

void connection_pool_close(connection_pool_t* cpt) {
	free(cpt->ip);
	int i;
	for(i = 0; i < cpt->maxConnection; i++) {
		if(cpt->cons[i] != NULL) {
			close(cpt->cons[i]->fd);
			free(cpt->cons[i]);
		}
	}
	free(cpt->cons);
	free(cpt);
}