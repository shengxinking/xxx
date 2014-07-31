
#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

/* the debug on/off macro */
#define PROXY_DEBUG		1

/* enable debug */
#ifdef PROXY_DEBUG

#define FLOW(level, fmt, args...)			\
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)

/* print debug message */
#define DBG(fmt, args...)	printf("proxyd: "fmt, ##args)

/* print buffer content */
#define DBGBUF(buf, size, fmt, args...) \
	printf("[buf]: "fmt, ##args);	\
	do {				\
		write(0, buf, size);	\
		fsync(0);		\
		printf("\n\n");		\
	} while (0);

/* disable debug */
#else

#define	FLOW(level, fmt, args...)
#define DBG(fmt, args...)
#define DBGBUF(buf, size, fmt, args...)

#endif

#define ERR(fmt, args...)			\
	fprintf(stderr, "[ERR]:%s:%d: " fmt,	\
		__FILE__, __LINE__, ##args)
#define	ERRSTR		strerror(errno)

#if 0

/* likely()/unlikely() for performance */
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif
#endif

#endif 

