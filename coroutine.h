#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define STACK_SIZE         (1024*1024)
#define DEFAULT_COROUTINE  16

#define COROUTINE_DEAD     0
#define COROUTINE_READY    1
#define COROUTINE_RUNNING  2
#define COROUTINE_SUSPEND  3

typedef struct schedule_s {
	char stack[STACK_SIZE];
	ucontext_t main;
	int nco;
	int cap;
	int running;
	struct coroutine_s **co;
} schedule_t;

typedef void (*coroutine_func)(schedule_t *, void *ptr);

typedef struct coroutine_s {
	coroutine_func func;
	void *ptr;
	ucontext_t ctx;
	schedule_t *sch;
	ptrdiff_t cap;
	ptrdiff_t size;
	int status;
	char *stack;
} coroutine_t;

// 默认使用堆模式
// 例如
// schedule_t *S;
// S = coroutine_open();
// ...
// coroutine_close(S);
schedule_t * coroutine_open();
void coroutine_close(schedule_t *S);
// 使用栈模式需要用户传入一个 schedule_t 类型的变量的地址 
// 例如
// schedule_t S;
// coroutine_open_stack(&S);
// ...
// coroutine_close_stack(&S);
void coroutine_open_stack(schedule_t *S);
void coroutine_close_stack(schedule_t *S);

int coroutine_new(schedule_t *S, coroutine_func func, void *ptr);
void coroutine_resume(schedule_t *S, int id);
int coroutine_status(schedule_t *S, int id);
int coroutine_running(schedule_t *S);
void coroutine_yield(schedule_t *S);

#endif
