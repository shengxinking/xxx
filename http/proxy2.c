#include <sys/resource.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#include <my_global.h>
#include <mysql.h>
#include "proxy.h"
#include "debug.h"
#include "packet.h"
#include "load_key.h"
#include "load_rule.h"

#include "log_db.h"
#include "cfg_db.h"


/**
 *	The global proxy variable.
 */
context_t		g_cfg;

extern int		_g_wafid;	/* number of real server */
MYSQL *m_db_log ;
MYSQL *m_db_cfg ;

/**
 *	The stop signal of proxy, it interrupt accept thread, and
 *	set @g_cfg.stop as 1, all other thread will check @g_cfg.stop
 *	to decide stop running.
 *
 *	No return.
 */


extern int load_waf_module();

static void 
_sig_int(int signo)
{
	if (signo != SIGINT)
		return;

	if (pthread_self() == g_cfg.main_tid) {
		printf("\n\n\nrecved stop signal SIGINT\n");
		g_cfg.stop = 1;
	}
	else {
		if (g_cfg.stop == 0)
			pthread_kill(g_cfg.main_tid, SIGINT);
	}
}


static int 
_proxy_init(context_t *ctx)
{
	int ret = 0;
	struct sigaction act;

	/* using SIGINT as stop signal */ 
	act.sa_handler = _sig_int;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGINT, &act, NULL)) {
		ERR("sigaction error: %s\n", ERRSTR);
		return -1;
	}

	/* ignore SIGPIPE signal */
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGPIPE, &act, NULL)) {
		ERR("sigaction error: %s\n", ERRSTR);
		return -1;
	}
	/* alloc packet pool */
	ctx->pktpool = mempool_alloc(MAX_PKTLEN, MAX_PKT_PRE, 1);
	if (!ctx->pktpool) {
		ERR("mempool_alloc failed\n");
		return -1;
	}

	DBG("alloc pktpool %p\n", ctx->pktpool);

	/* alloc connection pool */
	ctx->ssnpool = mempool_alloc(sizeof(connection_t), MAX_CONN_PRE, 1);
	if (!ctx->ssnpool) {
		ERR("mempool_alloc failed\n");
		return -1;
	}

	DBG("alloc ssnpool %p\n", ctx->ssnpool);

	pthread_mutex_init(&g_cfg.stat.lock, NULL);
	
	pthread_rwlock_init(&g_cfg.cfglock, NULL);
	
	pthread_mutex_init(&g_cfg.log_lock, NULL);

	apr_initialize();
	http_global_init();

	ret = init_fast_match();
	if (ret < 0) {
		printf("load fast match key error exit \n");
		return ret;
	}
		
	ret = load_rule();
	if (ret < 0) {
		printf("load rule  error exit \n");
		return ret;
	}

	load_waf_module();
	
	m_db_log = Db_Con_log();
	if (!m_db_log) {
		printf("load log db error exit \n");
		return -1;	
	}	
	
#if 0	
	m_db_cfg = Db_Con_cfg();
	if (!m_db_cfg) {
		printf("load cfg db error exit \n");
//		return -1;	
	}	
	check_cfg_update();
#endif
	ctx->main_tid = pthread_self();

	return 0;
}


static int 
_proxy_free(context_t *ctx)
{
	if (!ctx)
		return -1;

	if (ctx->pktpool) {
		DBG("pktpool: freed %d nalloced %d\n", 
		    ctx->pktpool->nfreed, ctx->pktpool->nalloced); 
		mempool_free(ctx->pktpool);
		ctx->pktpool = NULL;
	}

	if (ctx->ssnpool) {
		DBG("ssnpool: freed %d nalloced %d\n", 
		    ctx->ssnpool->nfreed, ctx->ssnpool->nalloced); 
		mempool_free(ctx->ssnpool);
		ctx->ssnpool = NULL;
	}


	return 0;
}


static int 
_proxy_loop(context_t *ctx)
{
	while (!ctx->stop) {
		sleep(5);
//		check_cfg_update();
	}

	return 0;
}

/**
 *	output the proxy's statistic data.
 */
static void 
_proxy_stat(context_t *ctx)
{

	printf("\n-------proxy statistic---------\n");
	printf("naccepts:   %lu\n", ctx->stat.naccept);
	printf("nclirecvs:  %lu\n", ctx->stat.nclirecv);
	printf("nclisends:  %lu\n", ctx->stat.nclisend);
	printf("nsvrrecvs:  %lu\n", ctx->stat.nsvrrecv);
	printf("nsvrsends:  %lu\n", ctx->stat.nsvrsend);
	printf("nclicloses: %lu\n", ctx->stat.ncliclose);
	printf("nsvrcloses: %lu\n", ctx->stat.nsvrclose);
	printf("nclierrors: %lu\n", ctx->stat.nclierror);
	printf("nsvrerrors: %lu\n", ctx->stat.nsvrerror);
	printf("-------------------------------\n");
}

/**
 *	The main entry of proxy.
 *
 *	Return 0 if success, -1 on error.
 */
int 
proxy_main2()
{
	int i;
	char buf[512];


	g_cfg.max = MAX_CUR_CON;
	g_cfg.dbglvl = 1;

	printf("\nproxyd: listen %s \n", 
	       ip_port_to_str(&g_cfg.httpaddr, buf, sizeof(buf)));
	printf("\t[real server]: %s\n", 
	       ip_port_to_str(&g_cfg.rsaddr, buf, sizeof(buf)));

	if (_proxy_init(&g_cfg)) {
		_proxy_free(&g_cfg);
		return -1;
	}

	for (i = 0; i < MAX_CUR_WORK; i++) {
		thread_create(&g_cfg.works[i], THREAD_WORK, 
			      work_run, i);
		usleep(50);
	}
	/* create threads */
	thread_create(&g_cfg.accept, THREAD_ACCEPT, accept_run, 0);


	if (thread_create(&g_cfg.clock, THREAD_CLOCK, clock_run,0)) {
                return -1;
	}


	_proxy_loop(&g_cfg);

	thread_join(&g_cfg.accept);
	for (i = 0; i < MAX_CUR_WORK; i++) {
		thread_join(&g_cfg.works[i]);
	}



	_proxy_stat(&g_cfg);

	_proxy_free(&g_cfg);

	return 0;
}

void 
proxy_stat_lock(void)
{
	pthread_mutex_lock(&g_cfg.stat.lock);
}

void 
proxy_stat_unlock(void)
{
	pthread_mutex_unlock(&g_cfg.stat.lock);
}




void 
proxy_cfg_unlock(void)
{
	pthread_rwlock_unlock(&g_cfg.cfglock);
}

void 
proxy_cfg_rlock(void)
{
	pthread_rwlock_rdlock(&g_cfg.cfglock);
}

void 
proxy_log_lock(void)
{
	pthread_mutex_lock(&g_cfg.log_lock);
}

void 
proxy_log_unlock(void)
{
	pthread_mutex_unlock(&g_cfg.log_lock);
}
