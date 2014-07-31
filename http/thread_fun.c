#include <string.h>
#include <errno.h>

#include "thread_fun.h"
#include "debug.h"

/**
 *	Create a new thread, the new thread function is @func, 
 *	the @func's argument is @info, the @type see above.
 *
 *	Return 0 if create thread success, -1 on error.
 */
int 
thread_create(thread_t *t_ctx, int type, void *(*func)(void *arg), int index)
{
	if (!t_ctx || !func) {
		ERR("thread_create: invalid parameter\n");
		return -1;
	}
	
	t_ctx->type = type;
	t_ctx->index = index;
	if (pthread_create(&t_ctx->tid, NULL, func, t_ctx)) {
		ERR("pthread_create: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


/**
 *	Wait a thread to exit.
 *
 *	Return 0 if success, -1 on error.
 */
int 
thread_join(thread_t *t_ctx)
{
	if (!t_ctx || !t_ctx->tid) {
		return -1;
	}
	
	if (pthread_join(t_ctx->tid, NULL)) {
		ERR("pthread_join: %s\n", strerror(errno));
		return -1;
	}

	DBG("thread %lu joined\n", t_ctx->tid);

	return 0;
}
