#include "filelock.h"

static void lock_init(struct flock *lock, short type, short whence, off_t start, off_t len);

void lock_init(struct flock *lock, short type, short whence, off_t start, off_t len)
{
	lock->l_type = type;
    lock->l_whence = whence;
    lock->l_start = start;
    lock->l_len = len;
}

filelock_mutex_t* filelock_mutex_create()
{
	filelock_mutex_t* fmt = (filelock_mutex_t*)malloc(sizeof(filelock_mutex_t));
	fmt->filename = (char*)malloc(sizeof(LOCK_FILE));
	strcpy(fmt->filename, LOCK_FILE);
	fmt->fd = open(fmt->filename, O_RDWR | O_CREAT, 0666);
	return fmt;
}

int filelock_mutex_rlock(filelock_mutex_t *fmt, int isblock)
{
	int error;
	struct flock lock;
	if(fmt == NULL) {
		return -1;
	}
	if(fmt->fd < 0) {
		return -1;
	}
	lock_init(&lock, F_RDLCK, SEEK_SET, 0, 0);
	if(isblock == LOCK_BLOCK) {
		if(fcntl(fmt->fd, F_SETLKW, &lock) != 0)
			return -1;
		else
			return 0;
	}
	else {
		error = fcntl(fmt->fd, F_SETLK, &lock);
		if(error != 0) {
			if(error == EAGAIN) {
				return 1;
			}
			else 
				return -1;
		}
		else
			return 0;
	}
	
}

int filelock_mutex_wlock(filelock_mutex_t *fmt, int isblock)
{
	int error;
	struct flock lock;
	if(fmt == NULL) {
		return -1;
	}
	if(fmt->fd < 0) {
		return -1;
	}
	lock_init(&lock, F_WRLCK, SEEK_SET, 0, 0);
	if(isblock == LOCK_BLOCK) {
		if(fcntl(fmt->fd, F_SETLKW, &lock) != 0)
			return -1;
		else
			return 0;
	}
	else {
		error = fcntl(fmt->fd, F_SETLK, &lock);
		if(error != 0) {
			if(error == EAGAIN) {
				return 1;
			}
			else 
				return -1;
		}
		else
			return 0;
	}
}

int filelock_mutex_unlock(filelock_mutex_t *fmt)
{
	struct flock lock;
	if(fmt == NULL) {
		return -1;
	}
	if(fmt->fd < 0) {
		return -1;
	}
	lock_init(&lock, F_UNLCK, SEEK_SET, 0, 0);
	if(fcntl(fmt->fd, F_SETLKW, &lock) != 0)
    {
        return -1;
    }
    return 0;
}

int filelock_mutex_locktest(filelock_mutex_t *fmt)
{
	struct flock lock;
	if(fmt == NULL) {
		return -1;
	}
	if(fmt->fd < 0) {
		return -1;
	}
	if(fcntl(fmt->fd, F_GETLK, &lock) != 0)
    {
        return -1;
    }

    if(lock.l_type == F_UNLCK)
        return 0;

    return lock.l_pid;
}

int filelock_mutex_close(filelock_mutex_t *fmt)
{
	if(fmt == NULL) {
		return -1;
	}
	close(fmt->fd);
	free(fmt->filename);
	free(fmt);
}