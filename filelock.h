#ifndef __FILELOCK_H__
#define __FILELOCK_H__

#ifdef __CPLUSPLUS              // 判断编译器是否为c++编译器，如果是则用c编译器编译
extern "C" {
#endif

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define LOCK_FILE     "lock.txt"
#define LOCK_UNBLOCK  0
#define LOCK_BLOCK    1

typedef struct filelock_mutex_s {
	int fd;
	char* filename;
}filelock_mutex_t; 

filelock_mutex_t* filelock_mutex_create();
int filelock_mutex_rlock(filelock_mutex_t *fmt, int isblock);
int filelock_mutex_wlock(filelock_mutex_t *fmt, int isblock);
int filelock_mutex_unlock(filelock_mutex_t *fmt);
int filelock_mutex_locktest(filelock_mutex_t *fmt);
int filelock_mutex_close(filelock_mutex_t *fmt);

#ifdef __cplusplus
}
#endif

#endif