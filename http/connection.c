
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>

#include "connection.h"
#include "mempool.h"

#define _connection_MAGIC		0x87654321

/* likely()/unlikely() for performance */
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif

/**
 *	Define debug MACRO to print debug information
 */
//#define _connection_DBG	1
#ifdef _connection_DBG
#define _SSN_ERR(fmt, args...)	fprintf(stderr, "%s:%d: "fmt,\
					__FILE__, __LINE__, ##args)
#else
#define _SSN_ERR(fmt, args...)
#endif

/**
 *	convert IP address(uint32_t) to dot-decimal string
 *	like xxx.xxx.xxx.xxx 
 */
#ifndef NIPQUAD
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif


/**
 * 	Find a free id in connection table @st. and update the 
 *	@st->freeid to next position..
 *
 *	Return the id if success, -1 on error.
 */
static int 
_st_get_free_id(connection_table_t *st)
{
	int id;
	
	assert(st);

	id = st->freeid;
	do {
		if (id >= (int)st->max)
			id = 0;

		if (st->table[id] == NULL) {
			st->freeid = id + 1;
			return id;
		}
		
		id++;
	} while (id != st->freeid);

	return -1;
}

int 
connection_tup_compare(connection_tup_t *tup1, connection_tup_t *tup2)
{
	if (unlikely(!tup1 || !tup2)) {
		_SSN_ERR("invalid argument\n");
		return -1;
	}

	/* inline mode */
	if (tup1->clifd > 0 && tup1->clifd > 0) {
		return tup1->id - tup2->id;
	}
	
	/* offline mode */
	return ip_port_compare(&tup1->cliaddr, &tup2->cliaddr);
}

connection_table_t *
connection_table_alloc(size_t max)
{
	connection_table_t *t = NULL;

	if (unlikely(max < 1)) {
		_SSN_ERR("invalid argument\n");
		return NULL;
	}

	/* alloc sesspool object */
	t = malloc(sizeof(connection_table_t));
	if (!t) {
		_SSN_ERR("malloc memory failed: %s\n", 
			 strerror(errno));
		return NULL;
	}
	memset(t, 0, sizeof(connection_table_t));

	/* alloc connection map */
	t->table = malloc(sizeof(connection_t *) * max);
	if (!t->table) {
		_SSN_ERR("malloc memory failed: %s\n", 
			 strerror(errno));
		free(t);
		return NULL;
	}
	memset(t->table, 0, sizeof(connection_t *) * max);
	
	t->max = max;
	t->nfreed = max;
	pthread_mutex_init(&t->lock, NULL);

	return t;
}

void 
connection_table_free(connection_table_t *t)
{
	if (unlikely(!t))
		return;

	if (t->table) {
		free(t->table);
	}

	free(t);
}

void 
connection_table_lock(connection_table_t *t)
{
	if (unlikely(!t))
		return;

	if (pthread_mutex_lock(&t->lock)) {
		_SSN_ERR("lock connection table failed\n");
	}
}

void 
connection_table_unlock(connection_table_t *t)
{
	if (unlikely(!t))
		return;

	if (pthread_mutex_unlock(&t->lock)) {
		_SSN_ERR("unlock connection table failed\n");
	}
}

int  
connection_table_add(connection_table_t *t, connection_t *s)
{
	int id;

	if (unlikely(!t || !t->table || !s)) {
		_SSN_ERR("invalid argument\n");
		return -1;
	}
	
	connection_table_lock(t);

	if (t->nfreed < 1) {
		connection_table_unlock(t);
		return -1;
	}

	id = _st_get_free_id(t);
	if (id < 0) {
		connection_table_unlock(t);
		_SSN_ERR("can't find a free id\n");
		return -1;
	}

	s->id = id;
	s->magic = _connection_MAGIC;
	t->table[id] = s;
	t->nfreed--;

	connection_table_unlock(t);

	return 0;
}

connection_t * 
connection_table_find(connection_table_t *t, int id)
{
	connection_t *s = NULL;

	if (unlikely(!t || !t->table || id < 0 || id >= (int)t->max)) {
		_SSN_ERR("invalid parameter\n");
		return NULL;
	}

	s = t->table[id];
	if (!s) {
		_SSN_ERR("invalid map %d, connection is NULL\n", id);
		return NULL;
	}

	assert(s->magic == _connection_MAGIC);

	return s;
}

int 
connection_table_del(connection_table_t *t, int id)
{
	connection_t *s = NULL;
	
	if (unlikely(!t || !t->table || id < 0 || id >= (int)t->max)) {
		_SSN_ERR("invalid argument\n");
		return -1;
	}

	/* lock the hash entry */
	connection_table_lock(t);
	
	s = t->table[id];
	if (!s) {
		connection_table_unlock(t);
		_SSN_ERR("free a not exist connection %d\n", id);
		return -1;
	}
	
	s->magic = 0;
	t->table[id] = NULL;
	t->nfreed++;

	connection_table_unlock(t);

	return 0;
}

void 
connection_table_print(connection_table_t *t)
{
	int i;

	if (unlikely(!t))
		return;

	pthread_mutex_lock(&t->lock);
		
	printf("sesstbl(%p) table %p max %lu, nfreed %lu\n", 
	       t, t->table, t->max, t->nfreed);
	
	for (i = 0; i < (int)t->max; i++) {
		if (t->table[i]) 
			printf("%d:", i);
			connection_print(t->table[i]);
	}

	pthread_mutex_unlock(&t->lock);
}

void 
connection_print(const connection_t *s)
{
	//char cstr[512];
	//char sstr[512];

	if (unlikely(!s))
		return;

	printf("\tconnection(%p) id %d, clifd %d, svrfd %d\n", 
	       s, s->id, s->clifd, s->svrfd);
#if 0
	printf("\t\t%s->%s\n", 
	       ip_port_to_str(&s->cliaddr, cstr, sizeof(cstr)), 
	       ip_port_to_str(&s->svraddr, sstr, sizeof(sstr)));
#endif
}



/*ptr is return value*/
char* get_pkt_buf(connection_t *conn, int *len, int dir)
{               
        packet_t *pkt;
        char *ptr = NULL;
        
		pkt = conn->cliq.pkt;
        ptr = pkt->data;
        *len = pkt->len;

        return ptr;
}
