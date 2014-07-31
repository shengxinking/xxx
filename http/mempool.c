

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "mempool.h"

/**
 *	Define some MACRO for print error message
 */
#define _OP_DBG
#ifdef _OP_DBG
#define _OP_ERR(f, a...)	fprintf(stderr, "%s:%d: "f,	\
					__FILE__, __LINE__, ##a)
#else
#define _OP_ERR(f, a...)
#endif

/*	The memory align macro */
#ifdef	__i386__
#define	_OP_ALIGN(n)	(((n + 3) / 4) * 4)
#else
#define	_OP_ALIGN(n)	(((n + 7) / 8) * 8)
#endif

/* Get pointer to _mempool_node accoring @mempool_node_t->data */
#define _OP_OFFSET(p)				\
	(void *)((char *)p - _OP_ALIGN(sizeof(_op_node_t)))


struct _op_node;

/**
 *	The memory cache, all memects stored in it.
 *
 */
typedef	struct _op_cache {
	struct _op_cache *next, **pprev;/* the list pointer */
	void		*pool;		/* the memory for memect */
	u_int32_t	magic;		/* magic nunber == pool's*/
	u_int32_t	capacity;	/* total number in cache */
	u_int32_t	nused;		/* number of used memect */
} _op_cache_t;


/**
 *	Struct _op_node is the header of each memory 
 *	unit in memory pool. 
 */
typedef struct _op_node {
	struct _op_node	*next, **pprev;	/* list pointer in cache */
	_op_cache_t	*cache;		/* the cache it belongs */
	mempool_t	*pool;		/* the pool it belongs */
	u_int32_t	magic;		/* magic number == pool's*/
	u_int32_t	is_used:1;	/* this node is used */
} _op_node_t;


/**
 *	Lock the memect pool @mp, it's for thread-safe
 *
 *	No return.
 */
static void 
_op_lock(mempool_t *op)
{
	assert(op);
	
	if (op->need_lock) {
		pthread_mutex_lock(&op->lock);
	}
}


/**
 *	Unlock the memect pool @op, it's for thread-safe
 *
 *	No return.
 */
static void 
_op_unlock(mempool_t *op)
{
	assert(op);

	if (op->need_lock) {
		pthread_mutex_unlock(&op->lock);
	}
}


/**
 *	Check system memory usage.
 *
 * 	Return 0 if can alloc memory, -1 if memory usage is
 *	too high
 */
static int 
_op_check_memory(void)
{
	return 0;
}


/**
 *	Alloc a memory cache, it can hold memPOOL_INS_SIZE memect.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_op_alloc_cache(mempool_t *op)
{
	_op_cache_t *cache = NULL;
	_op_node_t *node = NULL;
	int i;

	assert(op);

	if (_op_check_memory())
		return -1;

	cache = malloc(sizeof(_op_cache_t));
	if (!cache) {
		_OP_ERR("alloc memory for cache failed\n");
		return -1;
	}

	cache->pool = malloc(op->nodesize * op->incsize);
	if (!cache->pool) {
		return -1;
	}

	/* put all memory unit into free_list */
	for (i = 0; i < op->incsize; i++) {
		node = (_op_node_t *)((char *)cache->pool + i * op->nodesize);
		memset(node, 0, op->nodesize);
		node->magic = op->magic;
		node->cache = cache;
		node->pool = op;
	
		/* add to @mp->node_list */
		if (op->node_list)
			((_op_node_t *)op->node_list)->pprev = &node->next;
		node->pprev = (_op_node_t **)&op->node_list;
		node->next = op->node_list;
		op->node_list = node;
	}

	cache->magic = op->magic;
	cache->capacity = op->incsize;
	cache->nused = 0;
	op->nalloced += op->incsize;
	op->nfreed += op->incsize;

	/* add it to @op->cache_list */
	if (op->cache_list)
		((_op_cache_t *)op->cache_list)->pprev = &(cache->next);
	cache->next = op->cache_list;
	cache->pprev = (_op_cache_t **)&op->cache_list;
	op->cache_list = cache;

	return 0;
}


/**
 *	Delete cache @cache from memect pool @op's cache list.
 *	and remove all free nodes in @op->node_list.
 *
 *	No return.
 */
static void 
_op_free_cache(mempool_t *op, _op_cache_t *cache)
{
	_op_node_t *node;
	int i;

	assert(op);
	assert(cache);
	assert(cache->nused == 0);
	assert(cache->pprev);
	assert(cache->pool);
	assert(op->magic == cache->magic);
	
	if (cache->next)
		cache->next->pprev = cache->pprev;
	*cache->pprev = cache->next;

	cache->pprev = NULL;
	cache->next = NULL;

	for (i = 0; i < op->incsize; i++) {
		node = (_op_node_t *)((char *)cache->pool + i * op->nodesize);
		assert(node->is_used == 0);
		assert(node->pprev);

		/* delete it from @mp->node_list */
		if (node->next)
			node->next->pprev = node->pprev;
		*node->pprev = node->next;
	}

	op->nalloced -= op->incsize;
	op->nfreed -= op->incsize;

	assert(op->nalloced >= 0);
	assert(op->nfreed >= 0);

	free(cache->pool);
	free(cache);
}


/**
 *	Free cache list in memory pool @mp
 *
 *	No return.
 */
static void 
_op_free_cache_list(mempool_t *op)
{
	_op_cache_t *cache, *next;

	assert(op);

	cache = op->cache_list;
	
	while (cache) {
		next = cache->next;
		if (cache->pool)
			free(cache->pool);
		free(cache);
		cache = next;
	}

	op->cache_list = NULL;
}


/**
 *	print the cache information.
 *
 *	No return.
 */
static void 
_op_print_cache(const _op_cache_t *cache)
{
	assert(cache);

	printf("  cache(%p): magic %u, pool %p, capacity %u, "
	       "nused %u, pprev %p, next %p\n",
	       cache, cache->magic, cache->pool, cache->capacity, 
	       cache->nused, cache->pprev, cache->next);
}


/**
 *	Put a node into memory pool @mp
 *
 *	No return.
 */
static void 
_op_put_node(mempool_t *op, _op_node_t *node)
{
	_op_cache_t *cache;
	
	assert(op);
	assert(node);
	assert(node->cache);

	cache = node->cache;

	assert(op->magic == node->magic);
	assert(op->magic == cache->magic);
	assert(node->is_used == 1);
	
	/* add node to node_list */
	if (op->node_list) {
		((_op_node_t*)(op->node_list))->pprev = &node->next; 
	}
	node->pprev = (_op_node_t **)&op->node_list;
	node->next = op->node_list;
	op->node_list = node;
	
	node->is_used = 0;
	op->nfreed++;
	cache->nused--;

	assert(cache->nused >= 0);

	/**
	 * if all nodes in cache is freed and too many 
	 * free nodes, free this cache 
	 */
	if (cache->nused == 0  && op->nfreed > 2 * op->incsize)
		_op_free_cache(op, cache);

}


/**
 *	print the _mempool_node's to stdout
 *
 *	No return.
 */
static void 
_op_print_node(const _op_node_t *node)
{
	assert(node);
	
	printf("    node(%p): magic %u, is_used %x, cache %p, "
	       "pool %p, pprev %p, next %p\n",
	       node, node->magic, node->is_used, node->cache, 
	       node->pool, node->pprev, node->next);
}


mempool_t * 
mempool_alloc(size_t memsize, int incsize, int locked)
{
	mempool_t *op = NULL;

	if (memsize < 1)
		return NULL;

	if (incsize < 1)
		incsize = memPOOL_INC_SIZE;

	op = malloc(sizeof(mempool_t));
	if (!op) {
		_OP_ERR("malloc mempool error: %s\n", 
			strerror(errno));
		return NULL;
	}
	memset(op, 0, sizeof(mempool_t));

	op->magic = time(NULL);
	op->memsize = _OP_ALIGN(memsize);
	op->incsize = incsize;
	op->nodesize = op->memsize + _OP_ALIGN(sizeof(_op_node_t));

	/* using create time as magic number, so it's unique */
	if (_op_alloc_cache(op)) {
		free(op);
		return NULL;
	}

	/* initiate pthread lock for thread-safe */
	if (locked) {
		op->need_lock = 1;
		pthread_mutex_init(&op->lock, NULL);
	}

	return op;
}


void 
mempool_free(mempool_t *op)
{
	if (!op)
		return;

	_op_free_cache_list(op);

	free(op);
}


void * 
mempool_get(mempool_t *op)
{
	_op_node_t *node = NULL;

	if (!op)
		return NULL;

	_op_lock(op);

	/* no freed memory unit */
	if (op->nfreed == 0 && _op_alloc_cache(op)) {
		_op_unlock(op);
		return NULL;
	}

	/* get node from @mp->node_list */
	node = op->node_list;
	if (node->next)
		node->next->pprev = (_op_node_t **)&op->node_list;
	op->node_list = node->next;

	node->is_used = 1;
	node->next = NULL;
	node->pprev = NULL;
	
	assert(node->cache);
	node->cache->nused++;
	op->nfreed--;

	_op_unlock(op);

	return ((char *)node + _OP_ALIGN(sizeof(_op_node_t)));
}


int 
mempool_put1(mempool_t *op, void *ptr)
{
	_op_node_t *node = NULL;

	if (!op || !ptr)
		return -1;
	
	node = _OP_OFFSET(ptr);

	_op_lock(op);
	
	_op_put_node(op, node);
	
	_op_unlock(op);
	
	return 0;
}


int 
mempool_put(void *ptr)
{
	_op_node_t *node = NULL;
	mempool_t *op = NULL;

	if (!ptr)
		return -1;
	node = _OP_OFFSET(ptr);

	assert(node->pool);
	op = node->pool;

	_op_lock(op);
	
	_op_put_node(op, node);
	
	_op_unlock(op);
	
	return 0;
}



void 
mempool_print(mempool_t *op)
{
	_op_cache_t *cache;
	_op_node_t *node;

	if (!op)
		return;

	_op_lock(op);

	printf("pool(%p): magic %u, nalloced %u, nfreed: %u\n",
	       op, op->magic, op->nalloced, op->nfreed);

	cache = op->cache_list;
	while (cache) {
		_op_print_cache(cache);
		cache = cache->next;
	}

	node = op->node_list;
	while (node) {
		_op_print_node(node);
		node = node->next;
	}

	_op_unlock(op);
}

