// 文件名：threadpool.h
// 作者：丁燎原
// 创建时间：2016/1/8
// 功能：线程池，适用于c、c++
// 版本：
//      1、初始版本，基本功能实现
//      
# ifndef __THREADPOOL_H__
# define __THREADPOOL_H__

# ifdef __CPLUSPLUS              // 判断编译器是否为c++编译器，如果是则用c编译器编译
extern "C" {
# endif

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "dbg.h"

#define THREAD_NUM 8            

// 任务结构体，即任务链表的节点
typedef struct task_s {
	void (*taskFunc)(void *);   // 任务函数
	void *arg;                  // 任务函数的参数
	struct task_s *next;        // 指向下一个任务节点的指针
}task_t;

// 线程池结构体
typedef struct threadpool_s {
	pthread_mutex_t lock;       // 互斥锁
	pthread_cond_t cond;        // 条件变量
	pthread_t *pthreads;        // 工作线程数组指针
	task_t *head;               // 任务链表表头，表头不存储任何任务，是一个空头
	int pthreadNum;             // 线程个数
	int taskNum;                // 任务个数 
	int pthreadOnNum;           // 正在工作的线程个数
	int shutdown;               // 线程池状态，0：正在工作；1：强制结束，不等待任务链表为空；2：优雅结束，等待任务链表为空
}threadpool_t;

// 错误状态枚举
typedef enum {
    threadpool_invalid   = -1,
    threadpool_lock_fail = -2,
    threadpool_already_shutdown  = -3,
    threadpool_cond_broadcast    = -4,
    threadpool_thread_fail       = -5,   
}threadpool_error_t;

// 初始化线程池
// 输入：线程个数
// 输出：线程池结构体指针
threadpool_t *threadpoolInit(int ptNum);

// 向线程池任务链表中增加任务
// 输入：线程池指针，任务函数指针，任务函数参数
// 输出：执行状态
int threadpoolAdd(threadpool_t *tp, void (*func)(void *), void *arg);

// 销毁线程池
// 输入：线程池指针，销毁方式，可取1或2
// 输出：执行状态
int threadpoolDestroy(threadpool_t *tp, int destroyType);

#ifdef __cplusplus
}
# endif

# endif