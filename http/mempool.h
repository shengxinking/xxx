
#ifndef	_MEMPOOL_H
#define _MEMPOOL_H

#include <pthread.h>
#include <sys/types.h>

#define	memPOOL_INC_SIZE	1000	/* alloc mem number one time*/

/**
 *	Struct mempool is a memect pool, you can get free memect 
 *	unit from it(using @pktpool_get), or put a unused memect 
 *	to it(using @mempool_put).
 *
 *	All memect need have same size @memsize, so don't exceed 
 *	it's size when using it.
 */
typedef struct mempool {
	u_int32_t	magic;		/* magic number for verify */
	u_int16_t	memsize;	/* the memect memory size */
	u_int16_t	incsize;	/* inceament size */
	u_int16_t	nodesize;	/* the node memory size */
	u_int16_t	need_lock:1;	/* the mempool need locked */
	void		*cache_list;	/* the cache list */
	void		*node_list;	/* the freed node list */
	u_int32_t	nalloced;	/* number of alloced memects*/
	u_int32_t	nfreed;		/* number of freed memects */
	pthread_mutex_t	lock;		/* lock for thread safe */
} mempool_t;

/**
 *	Alloc a new memory pool, the each unit in memory pool size
 *	is @memsize, it provide @memnum units. If @locked is not zero,
 *	this memory pool is thread safed.
 *
 *	Return non-void pointer if success, NULL means error
 */
extern mempool_t*
mempool_alloc(size_t memsize, int incsize, int locked);

/**
 *	Free memory pool @pool when it's not used.
 *
 *	No return value
 */
extern void 
mempool_free(mempool_t *pool);

/**
 *	Get a free memory unit from memory pool @pool, the returned
 *	memory area size is @memsize when @pool is create. 
 *	
 *	Return non-void pointer if success, NULL means error.
 */
extern void*
mempool_get(mempool_t *pool);

/**
 *	Put a memory unit into memory pool @pool, so it can be used
 *	later. The returned memory unit need be one which is get 
 *	from the @pool early.
 *
 *	Return 0 if success, return -1 if error. 
 */
extern int 
mempool_put1(mempool_t *pool, void *data);

/**
 *	Put a memory unit into memory pool, the memory pool is hidden
 *      in @data, see implement.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
mempool_put(void *data);


/**
 *	Print the memory pool @pool in stdout.
 *
 *	No return value
 */
extern void 
mempool_print(mempool_t *pool);


#endif /* end of FZ_memPOOL_H */


