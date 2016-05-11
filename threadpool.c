#include "threadpool.h"

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
}threadpool_sd_t;

static int threadpoolFree(threadpool_t *tp);
static void *threadpoolWorker(void *arg);

threadpool_t *threadpoolInit(int ptNum)
{
	if (ptNum <= 0) {
		log_err("the arg of threadpool_init must greater than 0");
		return NULL;
	}

	threadpool_t *tp;
	if ((tp = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
		goto err;
	}

	tp->pthreadNum = 0;
	tp->taskNum = 0;
	tp->pthreadOnNum = 0;
	tp->shutdown = 0;
	tp->pthreads = (pthread_t *)malloc(sizeof(pthread_t) * ptNum);

	tp->head = (task_t *)malloc(sizeof(task_t));
	tp->head->taskFunc = NULL;
	tp->head->arg = NULL;
	tp->head->next = NULL;

	if ((tp->pthreads == NULL) || (tp->head == NULL)) {
		goto err;
	}

	if (pthread_mutex_init(&(tp->lock), NULL) != 0) {
		goto err;
	}

	if (pthread_cond_init(&(tp->cond), NULL) != 0) {
		pthread_mutex_lock(&(tp->lock));
		pthread_mutex_destroy(&(tp->lock));
		goto err;
	}
	int i;                       // 只有在c99中才可以在for循环中声明变量use option -std=c99 or -std=gnu99
	for (i = 0; i < ptNum; i++) {
		if (pthread_create(&(tp->pthreads[i]), NULL, threadpoolWorker, (void *)tp) != 0) {
			threadpoolDestroy(tp, 0);
			return NULL;
		}
		log_info("thread: %08lx started", tp->pthreads[i]);
		tp->pthreadNum++;
		tp->pthreadOnNum++;
	}

	return tp;

err:
	if (tp) {
		threadpoolFree(tp);
	}
	return NULL;
} 

int threadpoolFree(threadpool_t *tp)
{
	if (tp == NULL || tp->pthreadOnNum > 0) {
		return -1;
	}

	if(tp->pthreads) {
		free(tp->pthreads);
	}

	task_t *temp;
	while (tp->head->next) {
		temp = tp->head->next;
		tp->head->next = tp->head->next->next;
		free(temp);
	}
	if (tp->head) {
		free(tp->head);
	}     //????????????????????????????????

	return 0;
}

int threadpoolDestroy(threadpool_t *tp, int destroyType)
{
	int err = 0;

	if (tp == NULL) {
		log_err("pool == NULL");
		return threadpool_invalid;
	}

	if (pthread_mutex_lock(&(tp->lock)) != 0){
		return threadpool_lock_fail;
	}

	do {
		if (tp->shutdown) {
			err = threadpool_already_shutdown;
			break;
		}

		tp->shutdown = destroyType;

		if(pthread_cond_broadcast(&(tp->cond)) != 0) {
			err = threadpool_cond_broadcast;
			break;
		}

		if(pthread_mutex_unlock(&(tp->lock)) != 0) {
			err = threadpool_lock_fail;
			break;
		}

		int i;
		for (i = 0; i < tp->pthreadNum; i++) {
			if (pthread_join(tp->pthreads[i], NULL) != 0) {
				err = threadpool_thread_fail;
			}
			log_info("thread %08lx exit", tp->pthreads[i]);
		}

	}while(0);

	if (!err) {
        pthread_mutex_destroy(&(tp->lock));
        pthread_cond_destroy(&(tp->cond));
        threadpoolFree(tp);
    }

    return err;	
}

int threadpoolAdd(threadpool_t *tp, void (*func)(void *), void *arg)
{
	int err = 0;
	if (tp == NULL || func == NULL) {
		log_err("pool == NULL or func == NULL");
		return -1;
	}

	if (pthread_mutex_lock(&(tp->lock)) != 0) {
        log_err("pthread_mutex_lock");
        return -1;
    }

    if (tp->shutdown) {
        err = threadpool_already_shutdown;
        goto out;
    }

    task_t *task = (task_t *)malloc(sizeof(task_t));
    if (task == NULL) {
    	log_err("malloc task fail");
    	goto out;
    }

    task->taskFunc = func;
    task->arg = arg;
    task->next = tp->head->next;
    tp->head->next = task;

    tp->taskNum++;

    err = pthread_cond_signal(&(tp->cond));
    check(err == 0, "pthread_cond_signal");

out:
	if (pthread_mutex_unlock(&(tp->lock)) != 0) {
		log_err("pthread_mutex_unlock");
		return -1;
	}

	return err;
}

void *threadpoolWorker(void *arg)
{
	if (arg == NULL) {
        log_err("arg should be type zv_threadpool_t*");
        return NULL;
    }

    threadpool_t *tp = (threadpool_t *)arg;
    task_t *task = NULL;

    while(1) {
    	pthread_mutex_lock(&(tp->lock));

    	while((tp->taskNum == 0) && !(tp->shutdown)) {
    		pthread_cond_wait(&(tp->cond), &(tp->lock));
    	}

    	if(tp->shutdown == immediate_shutdown) {
    		pthread_mutex_unlock(&(tp->lock));
    		break;
    	}
    	else if((tp->shutdown == graceful_shutdown) && (tp->taskNum == 0)) {
    		pthread_mutex_unlock(&(tp->lock));
    		break;
    	}

    	task = tp->head->next;
    	if (task == NULL) {
    		continue;
    	}

    	tp->head->next = task->next;
    	tp->taskNum--;
    	pthread_mutex_unlock(&(tp->lock));

    	(*(task->taskFunc))(task->arg);
    	free(task);
    }

    tp->pthreadOnNum--;
    pthread_exit(NULL);

    return NULL;
}



