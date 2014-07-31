
#define	_GNU_SOURCE

#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "debug.h"
#include "thread_fun.h"
#include "proxy.h"
#include "connection.h"
#include "sock_util.h"
#include "http.h"
#include "http_basic.h"
#include "load_key.h"
#include "detect_module.h"


/**
 *	Init work thread, alloc resources.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_init(thread_t *t_ctx)
{
	work_t *wi;
	int maxfds = 0;

	assert(t_ctx);

	/* alloc work_t_ctx */
	wi = malloc(sizeof(work_t));
	if (!wi) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(wi, 0, sizeof(work_t));

	maxfds = (g_cfg.max / g_cfg.nwork) + 1;

	/* alloc memory for input queue */
	wi->inq = malloc(sizeof(workfd_t)  * maxfds);
	if (!wi->inq) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(wi->inq, 0, sizeof(workfd_t)  * maxfds);
	
	/* alloc memory for secondary input queue */
	wi->inq2 = malloc(sizeof(workfd_t *) * maxfds);
	if (!wi->inq2) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(wi->inq2, 0, sizeof(workfd_t *) * maxfds);

	wi->ninq = 0;
	wi->max = maxfds;

	pthread_mutex_init(&wi->lock, NULL);

	DBG("work(%d) alloc %d input queue\n", t_ctx->index, maxfds);
	
	/* alloc memory for epoll event */
	wi->events = malloc(maxfds * sizeof(struct epoll_event) * 2);
	if (!wi->events) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	wi->nevent = maxfds;


	/* alloc connection table */
	wi->sesstbl = connection_table_alloc(g_cfg.max);
	if (!wi->sesstbl) {
		ERR("connection_table_alloc failed\n");
		return -1;
	}

	DBG("work(%d) alloc %d epoll event\n", t_ctx->index, maxfds * 2);
	
	/* create send epoll fd */
	wi->send_epfd = epoll_create(maxfds);
	if (wi->send_epfd < 0) {
		ERR("epoll_create error %s\n", ERRSTR);
		return -1;
	}

	DBG("work(%d) create send epoll fd %d\n", t_ctx->index, wi->send_epfd);

	/* create send epoll fd */
	wi->recv_epfd = epoll_create(maxfds);
	if (wi->recv_epfd < 0) {
		ERR("epoll_create error %s\n", ERRSTR);
		return -1;
	}
	
	DBG("work(%d) create recv epoll fd %d\n", t_ctx->index, wi->recv_epfd);

	t_ctx->priv = wi;

	return 0;
}

/**
 *	Release work thread resource which alloced by _work_init()
 *
 *	No return.
 */
static void 
_work_free(thread_t *t_ctx)
{
	work_t *wi;

	assert(t_ctx);

	wi = t_ctx->priv;	
	if (!wi)
		return;

	if (wi->inq) {
		free(wi->inq);
		wi->inq = NULL;
		DBG("work(%d) freed input queue\n", t_ctx->index);
	}

	if (wi->inq2) {
		free(wi->inq2);
		wi->inq2 = NULL;
		DBG("work(%d) freed second input queue\n", t_ctx->index);
	}

	if (wi->events) {
		free(wi->events);
		wi->events = NULL;
		DBG("work(%d) free epoll event\n", t_ctx->index);
	}


	if (wi->sesstbl) {
		connection_table_free(wi->sesstbl);
		wi->sesstbl = NULL;
		DBG("work(%d) free connection table\n", t_ctx->index);
	}

	if (wi->send_epfd >= 0) {
		close(wi->send_epfd);
		DBG("work(%d) close send epoll %d\n", 
		    t_ctx->index, wi->send_epfd);
	}

	if (wi->recv_epfd >= 0) {
		close(wi->recv_epfd);
		DBG("work(%d) close recv epoll %d\n", 
		    t_ctx->index, wi->recv_epfd);
	}

	t_ctx->priv = NULL;
	free(wi);
}

/**
 *	Get a packet from pktpool if need, and put it to @q->pkt.
 *
 *	Return pointer to packet_t if success, NULL on error.
 */
static packet_t *
_work_get_packet(thread_t *t_ctx, connection_t *s, connection_queue_t *q)
{
	packet_t *pkt;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);
	assert(q);

#if 0
	pkt = q->pkt;
	if (pkt && pkt->capacity > pkt->len) {
		return pkt;
	}
#endif
	pkt = mempool_get(g_cfg.pktpool);
	if (!pkt) {
		ERR("can't get packet from pkt pool\n");
		return NULL;
	}
	memset(pkt, 0, sizeof(packet_t));
	s->nalloced++;
	q->pkt = pkt;
	pkt->len = 0;
	pkt->sendpos = 0;
	pkt->pos = 0;
	pkt->capacity = g_cfg.pktpool->memsize - sizeof(packet_t);
	q->pkt = pkt;

	if (q == &s->cliq) {
		FLOW(2, "work[%d]: ssn[%d] client %d get a packet(%p)\n", 
		     t_ctx->index, s->id, s->clifd, pkt);
	}
	else {
		FLOW(2, "work[%d]: ssn[%d] server %d get a packet(%p)\n",
		     t_ctx->index, s->id, s->svrfd, pkt);
	}

	return pkt;
}

/**
 *	Select a real server for connection @s, use round-robin algo.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_get_server(thread_t *t_ctx, connection_t *s)
{
	work_t *wi;
	struct epoll_event e;
	int fd;
	int wait;
	int *pair;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);

	wi = t_ctx->priv;

	if (s->svrfd > 0)
		return 0;

	pthread_mutex_lock(&wi->lock);
	fd = sk_tcp_client_nb(&g_cfg.rsaddr, &wait);
	if (fd < 0) {
		pthread_mutex_unlock(&wi->lock);
		s->is_svrerror = 1;
		ERR("connect server failed\n");
		return -1;
	}

	/* to next real server */
	pthread_mutex_unlock(&wi->lock);
	/* add it to send epoll to wait connect success */
	if (wait) {
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		e.events = EPOLLOUT;			
		if (epoll_ctl(wi->send_epfd, EPOLL_CTL_ADD, fd, &e)) {
			s->is_svrerror = 1;
			ERR("epoll add %d error %s\n", fd, ERRSTR);
			close(fd);
			return -1;
		}
		wi->nblocked ++;
		s->is_svrwait = 1;
		FLOW(1, "work[%d]: ssn[%d] client %d connect to server %d wait\n", 
		     t_ctx->index, s->id, s->clifd, fd);
	}
	/* add to recv epoll */
	else {
		pair = (int *)&e.data.u64;	
		pair[0] = fd;
		pair[1] = s->id;
		e.events = EPOLLIN;			
		if (epoll_ctl(wi->recv_epfd, EPOLL_CTL_ADD, fd, &e)) {
			ERR("epoll add %d error %s\n", fd, ERRSTR);
			s->is_svrerror = 1;
			close(fd);
			return -1;
		}
		FLOW(1, "work[%d]: ssn[%d] client %d connect to server %d success\n", 
		     t_ctx->index, s->id, s->clifd, fd);

	}

	s->svrfd = fd;

	return 0;
}

static connection_t * 
_work_alloc_connection(thread_t *t_ctx)
{
	work_t *wi;
	connection_t *s;

	assert(t_ctx);
	assert(t_ctx->priv);

	wi = t_ctx->priv;

	s = mempool_get(g_cfg.ssnpool);
	if (!s) {
		ERR("mempool_get failed\n");
		return NULL;
	}
	memset(s, 0, sizeof(connection_t));

	if (connection_table_add(wi->sesstbl, s)) {
		ERR("connection_table_add failed\n");
		mempool_put(s);
		return NULL;
	}

	return s;
}

/**
 *	Clean connection queue @q
 *
 *	No return.
 */
static void 
_work_clean_connection(thread_t *t_ctx, connection_t *s, connection_queue_t *q)
{
	packet_t *pkt;
	
	assert(t_ctx);
	assert(s);
	assert(q);
	
	pkt = q->pkt;
	if (pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 1 ssn[%d] free a packet %p\n", 
		     t_ctx->index, s->id, pkt);
		mempool_put(pkt);
	}

	pkt = q->blkpkt;
	if (pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 2 ssn[%d] free a packet %p\n", 
		     t_ctx->index, s->id, pkt);
		mempool_put(pkt);
	}

	pkt = pktq_out(&q->inq);
	while(pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 3 ssn[%d] free a packet %p\n", 
		     t_ctx->index, s->id, pkt);
		mempool_put(pkt);
		pkt = pktq_out(&q->inq);
	}

	pkt = pktq_out(&q->outq);
	while (pkt) {
		s->nalloced--;
		FLOW(2, "work[%d]: 4 ssn[%d] free a packet %p\n", 
		     t_ctx->index, s->id, pkt);
		mempool_put(pkt);
		pkt = pktq_out(&q->outq);
	}
}

/**
 *	Free connection @s, release all resource used by @s.
 *
 *	No return.
 */
static void 
_work_free_connection(thread_t *t_ctx, connection_t *s)
{
	work_t *wi;
	int fd;
	int epfd;

	assert(t_ctx);
	assert(t_ctx->priv);	
	assert(s);

	wi = t_ctx->priv;


	/* close client socket */
	if (s->clifd > 0) {
		fd = s->clifd;

		epfd = wi->send_epfd;
		if (s->is_cliblock && epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) {
			ERR("epoll del %d error %s\n", fd, ERRSTR);
		}
		
		epfd = wi->recv_epfd;
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) {
			ERR("epoll del %d error %s\n", fd, ERRSTR);
		}
		
		FLOW(1, "work[%d]: ssn[%d] client %d closed\n", 
		     t_ctx->index, s->id, fd);
		close(s->clifd);
	}

	/* close server socket */
	if (s->svrfd > 0) {

		fd = s->svrfd;
		epfd = wi->send_epfd;
		if (s->is_svrwait) { 
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
				ERR("epoll del %d error %s\n", fd, ERRSTR);
		}
		else {
			if (s->is_svrblock && 
			    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)) 
			{
				ERR("epoll del %d error %s\n", fd, ERRSTR);
			}
			
			epfd = wi->recv_epfd;
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL))
				ERR("epoll del %d error %s\n", fd, ERRSTR);

			FLOW(1, "work[%d]: ssn[%d] server %d closed\n", 
			     t_ctx->index, s->id, fd);
		}

		close(s->svrfd);
	}

	_work_clean_connection(t_ctx, s, &s->cliq);

	_work_clean_connection(t_ctx, s, &s->svrq);

/*caution: asser meme crash*/
//	assert(s->nalloced == 0);

	/* delete connection from connection pool */
	FLOW(1, "work[%d]: ssn[%d] deleted\n", t_ctx->index, s->id);

	connection_table_del(wi->sesstbl, s->id);
	DBGN("free connection \n");
	mempool_put(s);
#if 1
	proxy_stat_lock();
	g_cfg.stat.nowlive--;
	proxy_stat_unlock();
#endif
}

/**
 *	Get connection from input queue 
 *
 */
static int 
_work_get_connection(thread_t *t_ctx)
{
	work_t *wi;
	struct epoll_event e;
	connection_t *conn;
	workfd_t *wfd;
	int ninq;
	int *pair;
	int i;
	
	assert(t_ctx);
	assert(t_ctx->priv);

	wi = t_ctx->priv;

	/* get connection from input queue */
	pthread_mutex_lock(&wi->lock);
	ninq = wi->ninq;
	
	if (ninq > 0) {
		memcpy(wi->inq2, wi->inq, ninq * sizeof(workfd_t));
		memset(wi->inq, 0, ninq * sizeof(workfd_t));
		wi->ninq = 0;
	}
	pthread_mutex_unlock(&wi->lock);

	if (ninq < 1)
		return 0;

	for (i = 0; i < ninq; i++) {
	
		wfd = &wi->inq2[i];
		assert(wfd->fd > 0);
		
		conn = _work_alloc_connection(t_ctx);
		if (!conn) {
			close(wfd->fd);
			continue;
		}
		
		memset(&e, 0, sizeof(e));
		pair = (int *)&e.data.u64;
		pair[0] = wfd->fd;
		pair[1] = conn->id;
		e.events = EPOLLIN;
		if (epoll_ctl(wi->recv_epfd, EPOLL_CTL_ADD, wfd->fd, &e)) {
			printf("epoll add %d error %s\n", wfd->fd, ERRSTR);
			close(wfd->fd);
			_work_free_connection(t_ctx, conn);
			continue;
		}
		
		conn->clifd = wfd->fd;
        conn->cliaddr = wfd->addr;
        conn->cliport = wfd->port;
		
	}

	return 0;
}


static int 
_work_check_server(thread_t *t_ctx, connection_t *s)
{
	work_t *wi;
	struct epoll_event event;
	int ret = 0;
	int fd;
	int *pair;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);

	wi = t_ctx->priv;
	fd = s->svrfd;

	ret = sk_is_connected(fd);
	if (ret <= 0) {
		FLOW(1, "work[%d]: ssn[%d] server %d connect failed\n", 
		     t_ctx->index, s->id, fd);
		return -1;
	}
	s->is_svrwait = 0;
	wi->nblocked --;

	/* remove it from socket */
	if (epoll_ctl(wi->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
		ERR("epoll del  %d error %s\n", fd, ERRSTR);
		return -1;
	}

	/* add to recv epoll */
	pair = (int *)&event.data.u64;
	pair[0] = fd;
	pair[1] = s->id;
	event.events = EPOLLIN;
	if (epoll_ctl(wi->recv_epfd, EPOLL_CTL_ADD, fd, &event)) {
		ERR("epoll add %d error %s\n", fd, ERRSTR);
		return -1;
	}

	FLOW(1, "work[%d]: ssn[%d] client %d connect to server %d success\n", 
	     t_ctx->index, s->id, s->clifd, fd);

	return 0;
}


static int 
_work_recvfrom_client(thread_t *t_ctx, connection_t *s)
{
	packet_t *pkt;
	int fd;
	int n;
	int blen = 0;
	int close = 0;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);
	assert(s->clifd >= 0);
	
	fd = s->clifd;

	/* get packet to restore recved data */
	pkt = _work_get_packet(t_ctx, s, &s->cliq);
	if (!pkt)
		return -1;
	
	blen = pkt->capacity - pkt->len;
/*caution: packet is too large ,will crash*/
	assert(blen > 0);
	n = sk_recv(fd, pkt->data, blen, &close);
	if (n < 0) {
		s->is_clierror = 1;
		FLOW(1, "work[%d]: ssn[%d] client %d recv error\n", 
		     t_ctx->index, s->id, fd);
		return -1;
	}
	
	pkt->len += n;
#if 1
	proxy_stat_lock();
	g_cfg.stat.nclirecv += n;
	proxy_stat_unlock();
#endif
	FLOW(1, "work[%d]: ssn[%d] client %d recv %d bytes\n", 
	     t_ctx->index, s->id, fd, n);

	if (pkt->len == 0) {
		DBGN("pkt len is 0,no packet\n");
		mempool_put(pkt);
		s->cliq.pkt = NULL;
		s->nalloced--;

		FLOW(2, "work[%d]: ssn[%d] client %d free packet %p\n", 
		     t_ctx->index, s->id, fd, pkt);
	}

	if (close) {
		s->is_cliclose = 1;
		FLOW(1, "work[%d]: ssn[%d] client %d recv closed\n", 
		     t_ctx->index, s->id, fd);
	}

	return 0;
}

static int 
_work_recvfrom_server(thread_t *t_ctx, connection_t *s)
{
	packet_t *pkt;
	int n;
	int blen = 0;
	char *ptr;
	int fd;
	int close = 0;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);
	assert(s->svrfd >= 0);
	
	fd = s->svrfd;

	pkt = _work_get_packet(t_ctx, s, &s->svrq);
	if (!pkt) {
		return -1;
	}

	/* recv data */
	ptr = pkt->data ;
	blen = pkt->capacity; 
	assert(blen > 0);
	n = sk_recv(fd, ptr, blen, &close);
	if (n < 0) {
		s->is_svrerror = 1;
		FLOW(1, "work[%d]: ssn[%d] server %d recv error\n", 
		     t_ctx->index, s->id, fd);
		return -1;
	}
	
	pkt->len += n;

#if 1	
	proxy_stat_lock();
	g_cfg.stat.nsvrrecv += n;
	proxy_stat_unlock();
#endif

	if (pkt->len == 0) {
		mempool_put(pkt);
		s->svrq.pkt = NULL;
		s->nalloced--;
		FLOW(2, "work[%d] ssn[%d] server %d free packet %p\n", 
		     t_ctx->index, s->id, fd, pkt);
	}

	if (close) {
		s->is_svrclose = 1;
		FLOW(1, "work[%d]: ssn[%d] server %d closed\n", 
		     t_ctx->index, s->id, fd);
	}

	FLOW(1, "work[%d]: ssn[%d] server %d recv %d bytes\n", 
	     t_ctx->index, s->id, fd, n);

	return 0;
}



/**
 *	Parse client data
 *
 *	Return 0 if parse success, -1 on error.
 */
static int 
_work_parse_client(thread_t *t_ctx, connection_t *conn)
{
	packet_t *pkt;
	int len, ret = 0;
	void *ptr;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(conn);

	/* client is closed */
	if (!conn->cliq.pkt)
		return 0;

	pkt = conn->cliq.pkt;
	//ptr = pkt->data + pkt->recvpos;
//	len = pkt->len - pkt->recvpos;
	ptr = pkt->data;
	len = pkt->len;
	//DBGN("(%d):%s\n",len,pkt->data);	
	if (0) { 
	ret = do_http_parse_test(&(conn->http_ctx), pkt->data, pkt->len);
	if (-1 == ret) {
		return 0;
	}
	pthread_rwlock_rdlock(&g_cfg.cfglock);
	ret = module_detect_entry(&(conn->http_ctx), conn);
	pthread_rwlock_unlock(&g_cfg.cfglock);
	}
#if	1 
	if (ret == RET_FAIL) {
		return -1;
	}
#endif
	conn->cliq.pkt = NULL;
	pktq_in(&conn->cliq.outq, pkt);

	FLOW(1, "work[%d]: conn[%d] client %d move packet %p to output\n", 
	     t_ctx->index, conn->id, conn->clifd, pkt);

	return 0;
}

/**
 *	Parse server data.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_work_parse_server(thread_t *t_ctx, connection_t *s)
{
	packet_t *pkt;
//	int len;
//	void *ptr;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);
	
	/* server is closed */
	if (!s->svrq.pkt)
		return 0;

	pkt = s->svrq.pkt;
//	ptr = pkt->data + pkt->recvpos;
//	len = pkt->len - pkt->recvpos;

#if 0
	if (len > 0) {
		if (http_parse_response(&s->svrhttp, ptr, len, 0)) {
			s->flags |= connection_SVRERROR;
			ERR("http response parse error\n");
			return -1;
		}
	}
	/* server is closed */
	else {
		if (s->svrhttp.state != HTTP_STE_FIN) {
			ERR("http response not fin before closed\n");
			return -1;
		}
	}
	
	FLOW(1, "work[%d] ssn %d server %d http parse success\n", 
	     t_ctx->index, s->id, s->svrfd);
	
	/* move packet to output queue for send */
	if (pkt->len == pkt->capacity ||
	    s->svrhttp.state == HTTP_STE_FIN) 
	{
		s->svrcache.current = NULL;
		pktqueue_in(&s->svrcache.output, pkt);

		FLOW(1, "work[%d] ssn %d server %d move packet %p to output\n", 
		     t_ctx->index, s->id, s->svrfd);	
	}

#endif
	s->svrq.pkt = NULL;
	pktq_in(&s->svrq.outq, pkt);

	FLOW(1, "work[%d]: ssn[%d] server %d move packet %p to output\n", 
	     t_ctx->index, s->id, s->svrfd, pkt);

	return 0;
}



static int 
_work_sendto_client(thread_t *t_ctx, connection_t *s)
{
	work_t *wi;
	packet_t *pkt;
	struct epoll_event e;
	void *ptr;
	int *pair;
	int len;
	int n;
	int fd;
	int len1;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);
	assert(s->clifd > -1);

	wi = t_ctx->priv;
	fd = s->clifd;

	if (s->svrq.outq.len < 1 && ! s->svrq.blkpkt)
		return 0;

	/* check block packet first */
	if (s->svrq.blkpkt) {
		pkt = s->svrq.blkpkt;
		s->svrq.blkpkt = NULL;
	}
	else {
		pkt = pktq_out(&s->svrq.outq);
	}

	while (pkt) {
		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;

		len1 = len;
//		if (len > 100) len1 = len - 100;

		n = sk_send(fd, ptr, len1);
		if (n < 0) {
			FLOW(1, "work[%d]: ssn[%d] server %d sendto client %d failed\n", 
			     t_ctx->index, s->id, s->svrfd, fd);
			s->is_clierror = 1;
			s->nalloced--;
			mempool_put(pkt);
			return -1;
		} else if (n < len) {

			pkt->sendpos += n;			
			if (s->is_cliblock)
				break;
			
			/* add it to send epoll and set block */
			memset(&e, 0, sizeof(e));
			e.events = EPOLLOUT;
			pair = (int *)&e.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_ADD, fd, &e)) {
				ERR("epoll add %d error %s\n", fd, ERRSTR);
				mempool_put(pkt);
				s->nalloced--;
				return -1;
			}
			s->svrq.blkpkt = pkt;
			s->is_cliblock = 1;
			wi->nblocked++;
			FLOW(1, "work[%d]: ssn[%d] server %d sendto client %d blocked(%d:%d)\n",
			     t_ctx->index, s->id, s->svrfd, fd, len, n);
			break;
		}

		if (s->is_cliblock) {
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				ERR("epoll del %d error %s\n", fd, ERRSTR);
				mempool_put(pkt);
				s->nalloced--;
				FLOW(2, "work[%d]: ssn[%d] server %d free a packet %p\n", 
				     t_ctx->index, s->id, s->svrfd, pkt);
				return -1;
			}
			s->is_cliblock = 0;
			wi->nblocked--;
		}

		FLOW(1, "work[%d]: ssn[%d] server %d sendto client %d %d bytes\n", 
		     t_ctx->index, s->id, s->svrfd, s->clifd, len);

		mempool_put(pkt);
		s->nalloced--;

		FLOW(2, "work[%d]: ssn[%d] server %d free packet\n", 
		     t_ctx->index, s->id, s->svrfd);
		
		pkt = pktq_out(&s->svrq.outq);
	}
	
	return 0;

}

/**
 *	Send client data to server.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_sendto_server(thread_t *t_ctx, connection_t *s)
{
	work_t *wi;
	packet_t *pkt;
	struct epoll_event event;
	int fd;
	void *ptr;
	int len;
	int n;
	int *pair;
	int len1;

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);

	wi = t_ctx->priv;

	if (s->cliq.outq.len < 1 && ! s->cliq.blkpkt)
		return 0;

	if (_work_get_server(t_ctx, s)) 
		return -1;

	if (s->is_svrwait) {
		return 0;
	}

	fd = s->svrfd;

	/* send block packet first */
	if (s->cliq.blkpkt) {
		pkt = s->cliq.blkpkt;
		s->cliq.blkpkt = NULL;
	}
	else {
		pkt = pktq_out(&s->cliq.outq);
	}

	while (pkt) {

		/* send packet to server */
		ptr = pkt->data + pkt->sendpos;
		len = pkt->len - pkt->sendpos;
		len1 = len;
//		if (len > 100) len1 = len - 100;
		n = sk_send(fd, ptr, len1);
		/* send error */
		if (n <= 0) {
			s->is_svrerror = 1;
			mempool_put(pkt);
			s->nalloced--;
			FLOW(1, "work[%d]: ssn[%d] client %d sendto server %d error\n", 
			     t_ctx->index, s->id, s->clifd, fd);
			return -1;
		}
		/* send not finished, block client */
		else if (n < len) {

			pkt->sendpos = pkt->sendpos + n;			
			if (s->is_svrblock)
				break;

			/* add it to send epoll */
			pair = (int *)&event.data.u64;
			pair[0] = fd;
			pair[1] = s->id;
			event.events = EPOLLOUT;			
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_ADD, fd, &event)) {
				ERR("epoll add %d error %s\n", fd, ERRSTR);
				s->is_svrerror = 1;
				mempool_put(pkt);
				s->nalloced--;
				FLOW(2, "work[%d]: ssn[%d] client %d free a packet %p\n", 
				     t_ctx->index, s->id, s->clifd, pkt);
				
				return -1;
			}
			s->cliq.blkpkt = pkt;
			s->is_svrblock = 1;
			wi->nblocked++;
			FLOW(1, "work[%d]: ssn[%d] client %d sendto server %d %d:%d blocked\n", 
			     t_ctx->index, s->id, s->clifd, fd, len, n);
			break;
		}

		FLOW(1, "work[%d]: ssn[%d] client %d sendto server %d %d bytes\n", 
		     t_ctx->index, s->id, s->clifd, fd, len);

		/* clear block flags because send success */
		if (s->is_svrblock) {
			if (epoll_ctl(wi->send_epfd, EPOLL_CTL_DEL, fd, NULL)) {
				ERR("epoll del %d error %s\n", fd, ERRSTR);
				s->is_svrerror = 1;
				mempool_put(pkt);
				s->nalloced--;
				return -1;
			}
			s->is_svrblock = 0;
			wi->nblocked--;
		}

		s->nalloced--;
		mempool_put(pkt);

		pkt = pktq_out(&s->cliq.outq);
	}

	return 0;
}

/**
 *	Process client request, include recv client data, parse client data, 
 *	and send client data to server 
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_work_process_client(thread_t *t_ctx, connection_t *s)
{

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);
	assert(s->clifd >= 0);


	if (_work_recvfrom_client(t_ctx, s)) {
		_work_free_connection(t_ctx, s);
		return -1;
	}
       
	if (_work_parse_client(t_ctx, s)) {
		_work_free_connection(t_ctx, s);
		return -1;
	}
		
	if (_work_sendto_server(t_ctx, s)) {
		_work_free_connection(t_ctx, s);
		return -1;
	}

	if (s->is_clierror || s->is_cliclose) {
		_work_free_connection(t_ctx, s);
		return -1;
	}

	return 0;
}

/**
 *	Process server data, include recv data data, parse server data, and
 *	send server data to client.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_work_process_server(thread_t *t_ctx, connection_t *s)
{

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(s);
	assert(s->svrfd > -1);


	if (_work_recvfrom_server(t_ctx, s)) {
		_work_free_connection(t_ctx, s);
		return -1;
	}

	if (_work_parse_server(t_ctx, s)) {
		_work_free_connection(t_ctx, s);
		return -1;
	}

	if (_work_sendto_client(t_ctx, s)) {
		_work_free_connection(t_ctx, s);
		return -1;
	}

	if (s->is_svrerror || s->is_svrclose) {
		_work_free_connection(t_ctx, s);
	}

	return 0;
}


/**
 *	Recv data from socket 
 *
 *	Return 0 if success, -1 on error.
 */
static void 
_work_recv_data(thread_t *t_ctx)
{
	work_t *wi;
	connection_t *s;
	struct epoll_event *e;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;

	assert(t_ctx);
	assert(t_ctx->priv);

	wi = t_ctx->priv;

	nfds = epoll_wait(wi->recv_epfd, wi->events, 
			  wi->nevent, WORK_TIMEOUT);
	
	if (nfds < 0) {
		if (errno == EINTR)
			return;
		ERR("epoll_wait error %s", ERRSTR);
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {
		
		e = &wi->events[i];

		pair = (int *)&e->data.u64;
		fd = pair[0];
		id = pair[1];
		s = connection_table_find(wi->sesstbl, id);
		if (!s) {
			continue;
		}
		
		if (e->events & EPOLLERR) {
			_work_free_connection(t_ctx, s);
			continue;
		}

		if (s->clifd == fd) {
			if (!s->is_svrblock)
				_work_process_client(t_ctx, s);
		}
		else if (s->svrfd == fd) {
			if (!s->is_cliblock)
				_work_process_server(t_ctx, s);
		}
		else {
			ERR("the wrong fd %d is in connection %d\n", fd, id);
		}
	}

}


static void  
_work_send_data(thread_t *t_ctx) 
{
	work_t *wi;
	connection_t *s;
	struct epoll_event *e;
	int *pair;
	int fd;
	int id;
	int nfds;
	int i;

	assert(t_ctx);
	assert(t_ctx->priv);

	wi = t_ctx->priv;

	assert(wi->nblocked >= 0);

	if (wi->nblocked < 1)
		return;

	nfds = epoll_wait(wi->send_epfd, wi->events, 
			  wi->nevent, WORK_TIMEOUT);
	
	if (nfds < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return;

		ERR("epoll_wait error: %s", strerror(errno));
		return;
	}

	if (nfds == 0)
		return;

	for (i = 0; i < nfds; i++) {

		e = &wi->events[i];

		/* recv data */
		pair = (int *)&e->data.u64;
		fd = pair[0];
		id = pair[1];
		s = connection_table_find(wi->sesstbl, id);
		if (!s) {
			continue;
		}

		if (e->events & EPOLLERR) {
			_work_free_connection(t_ctx, s);
			continue;
		}

		if (s->clifd == fd) {
			if (_work_sendto_client(t_ctx, s))
				_work_free_connection(t_ctx, s);
		}
		else if (s->svrfd == fd) {
			if (s->is_svrwait && _work_check_server(t_ctx, s))
				_work_free_connection(t_ctx, s);

			if (_work_sendto_server(t_ctx, s)) 
				_work_free_connection(t_ctx, s);
		}
		else {
			ERR("the error fd %d is in connection %d", fd, id);
		}
	}
}




static int 
_work_loop(thread_t *t_ctx)
{
	while (!g_cfg.stop) {
		_work_get_connection(t_ctx);
		
		_work_send_data(t_ctx);

		_work_recv_data(t_ctx);
	}

	return 0;
}

/**
 *	The main function of work thread. 
 *
 *	Return NULL always.
 */
void *
work_run(void *arg)
{
	thread_t *t_ctx;

	t_ctx = arg;
	if (!t_ctx) {		
		ERR("the work thread need argument\n");

		if (g_cfg.main_tid > 0) {
			pthread_kill(g_cfg.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	DBG("work(%d) started\n", t_ctx->index);

	if (_work_init(t_ctx)) {
		if (g_cfg.main_tid > 0) {
			pthread_kill(g_cfg.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	_work_loop(t_ctx);

	_work_free(t_ctx);

	DBG("work(%d) stoped\n", t_ctx->index);

	pthread_exit(0);
}

/**
 *	Assign client @fd to work thread @index, the @index
 *	is the index of thread in @g_cfg.works, it's not
 *	the thread id. the client's IP is @ip, port is @port.
 *
 *	Return 0 if success, -1 on error.
 */
int 
work_add_connection(int index, workfd_t *wfd)
{
	work_t *wi;
	int ninq = 0;

	if (index < 0 || index >= g_cfg.nwork)
		return -1;

	if (!wfd)
		return -1;

	wi = g_cfg.works[index].priv;
	if ((!wi))
		return -1;

	/* locked */
	pthread_mutex_lock(&wi->lock);

	/* check array parameter */
	if (!wi->inq || wi->max < 1 || 
	    wi->ninq < 0 || wi->ninq >= wi->max) 
	{
		ERR("the work(%d) inq have problem\n", index);
		pthread_mutex_unlock(&wi->lock);
		return -1;
	}

	/* put client t_ctxmation */
	ninq = wi->ninq;
	wi->inq[ninq].fd = wfd->fd;
	wi->inq[ninq].is_ssl = wfd->is_ssl;
	wi->inq[ninq].addr = wfd->addr;
    wi->inq[ninq].port = wfd->port;
	wi->ninq++;

	/* unlock */
	pthread_mutex_unlock(&wi->lock);

	return 0;
}




