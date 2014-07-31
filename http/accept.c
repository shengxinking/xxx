
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/poll.h>

#include "debug.h"
#include "thread_fun.h"
#include "sock_util.h"
#include "proxy.h"

#define	NDEBUG		1

/**
 *	Alloc and initate accept thread global resource
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_init(thread_t *t_ctx)
{
	accept_t *ai;

	if (!t_ctx) {
		ERR("invalid param\n");
		return -1;
	}

	ai = malloc(sizeof(accept_t));
	if (!ai) {
		ERR("malloc error %s\n", ERRSTR);
		return -1;
	}
	memset(ai, 0, sizeof(accept_t));

	ai->httpfd = sk_tcp_server(&g_cfg.httpaddr);
	if (ai->httpfd < 0) {
		ERR("can't creat HTTP listen socket\n");
		return -1;
	}
	sk_set_nonblock(ai->httpfd, 1);
	DBG("accept(%d) create HTTP socket %d\n", t_ctx->index, ai->httpfd);

#if 0
	ai->httpsfd = sk_tcp_server(&g_cfg.httpsaddr);
	if (ai->httpsfd < 0) {
		ERR("can't create HTTPS listen socket\n");
		return -1;
	}
	sk_set_nonblock(ai->httpsfd, 1);
	DBG("accept(%d) create HTTPS socket %d\n", t_ctx->index, ai->httpsfd);
#endif

	t_ctx->priv = ai;

	return 0;
}

/**
 *	Release resource alloced by @_accept_initiate.
 *
 *	No return.
 */
static void 
_accept_release(thread_t *t_ctx)
{
	accept_t *ai;

	if (!t_ctx || !t_ctx->priv) {
		return;
	}

	ai = t_ctx->priv;
	
	if (ai->httpfd > 0) {
		close(ai->httpfd);
		DBG("accept close HTTP socket %d\n", ai->httpfd);
	}

	if (ai->httpsfd > 0) {
		close(ai->httpsfd);
		DBG("accept close HTTPS socket %d\n", ai->httpsfd);
	}

	free(ai);
	t_ctx->priv = NULL;
}

/**
 *	Accept a client connect, and assign it to a work thread.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_client(thread_t *t_ctx, int fd, int ssl, ip_port_t *dip)
{
	accept_t *ai;
	//ip_port_t ip;
	int clifd;
	workfd_t wfd;
	char buf[512], buf1[512];

	assert(t_ctx);
	assert(t_ctx->priv);
	assert(dip);

	ai = t_ctx->priv;

//	clifd = sk_accept(fd, &ip);
	clifd = sk_accept(fd, &wfd.addr, &wfd.port); 
	if (clifd < 0) {
		ERR("accept client error: %s\n", ERRSTR);
		return -1;
	}

	/* check concurrency is exceed or not */
	if (g_cfg.stat.nlive >= g_cfg.max) {
		close(clifd);
		return -1;
	}
	
	g_cfg.stat.naccept++;
//	if (ssl) g_cfg.stat.nhttps++;
//	else g_cfg.stat.nhttp++;
	
	FLOW(1, "accept[%d]: client %d accept (%s->%s)\n",  
	     t_ctx->index, clifd, 
	     ip_addr_to_str(&wfd.addr, buf, sizeof(buf)), 
	     ip_port_to_str(dip, buf1, sizeof(buf1)));
	
	/* assigned client to recv thread */
	wfd.fd = clifd;
	wfd.is_ssl = ssl ? 1 : 0;
	if (work_add_connection(ai->index, &wfd)) {
		printf("accept[%d] client %d add to work %d failed\n", 
		    t_ctx->index, clifd, ai->index);
		close(clifd);
		return -1;
	}
	
	FLOW(1, "accept[%d]:assign to work %d\n", 
	     t_ctx->index,  ai->index);

#if 1
	/* update statistic data */
	proxy_stat_lock();

	g_cfg.stat.nlive++;
	g_cfg.stat.nowlive++;
	if (ssl) g_cfg.stat.nhttpslive++;
	else g_cfg.stat.nhttplive++;
	
	proxy_stat_unlock();
#endif
	/* rotate work index */
	ai->index++;
	if (ai->index >= g_cfg.nwork)
		ai->index = 0;
		
	return 0;
}



/**
 *	Accept connection from client, and assign the client 
 *	fd to work thread.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_accept_loop(thread_t*t_ctx)
{
	accept_t *ai;
	struct pollfd pfds[2];
	int npfd = 0;
	int n;
	int i;

	assert(t_ctx);
	assert(t_ctx->priv);

	ai = t_ctx->priv;

	/* add http fd */
	if (ai->httpfd > 0) {
		pfds[npfd].fd = ai->httpfd;
		pfds[npfd].events = POLLIN;
		npfd++;
	}

	/* add https fd */
	if (ai->httpsfd > 0) {
		pfds[npfd].fd = ai->httpsfd;
		pfds[npfd].events = POLLIN;
		npfd++;
	}

	while (!g_cfg.stop) {
		
		n = poll(pfds, npfd, ACCEPT_TIMEOUT);
		
		if (n < 0) {
			ERR("poll error: %s\n", ERRSTR);
			continue;
		}
		if (n == 0) {
			continue;
		}

		for (i = 0; i < n; i++) {
			if (pfds[i].fd == ai->httpfd) {
				_accept_client(t_ctx, ai->httpfd, 0, 
					       &g_cfg.httpaddr);
			}
			else if (pfds[i].fd == ai->httpsfd) {
				_accept_client(t_ctx, ai->httpsfd, 1, 
					       &g_cfg.httpsaddr);
			}
			else {
				ERR("unknowed fd %d in poll\n", pfds[i].fd);
			}
		}
	}
	
	return 0;
}


void * 
accept_run(void *args)
{
	thread_t *t_ctx = NULL;

	usleep(200);
	
	t_ctx = args;
	assert(t_ctx);

	DBG("accept(%d) thread start\n", t_ctx->index);

	if (_accept_init(t_ctx)) {
		if (g_cfg.main_tid > 0) {
			pthread_kill(g_cfg.main_tid, SIGINT);
		}
		pthread_exit(0);
	}

	_accept_loop(t_ctx);

	_accept_release(t_ctx);

	DBG("accept(%d) thread exited\n", t_ctx->index);

	pthread_exit(0);
}




