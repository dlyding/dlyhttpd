#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define COL(x)  "\033[;" #x "m"  
#define RED        COL(31)  
#define GREEN      COL(32)  
#define YELLOW     COL(33)  
#define BLUE       COL(34)  
#define MAGENTA    COL(35)  
#define CYAN       COL(36)  
#define WHITE      COL(0)  
#define DEFAULT    "\033[0m" 

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, YELLOW"DEBUG"DEFAULT" %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, RED"[ERROR]"DEFAULT" (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, MAGENTA"[WARN]"DEFAULT" (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, GREEN"[INFO]"DEFAULT" (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); /* exit(1); */ }

#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); /* exit(1); */}

#endif
