
#ifndef PROXY_H
#define PROXY_H

#include <sys/types.h>
#include <pthread.h>

#include "ip_addr.h"
#include "mempool.h"
#include "thread_fun.h"
#include "connection.h"

#define TProxy 1




/* the max work thread number, real server number */
#define MAX_REALSVR	20
#define	MAX_WORKNUM	20
#define	MAX_PKTLEN	4096

#define MAX_PKT_PRE   10000
#define MAX_CONN_PRE  10000

#define MAX_CUR_CON  50000
#define MAX_CUR_WORK 4 


typedef struct proxy_stat {
	/* the session information */
	u_int64_t	nhttp;		/* total http session number */
	u_int64_t	nhttps;		/* total https session number */
	u_int64_t	naccept;	/* total session number (http+https) */
	u_int64_t	nhttplive;	/* live http session number */
	u_int64_t	nhttpslive;	/* live https session number */
	u_int64_t	nlive;		/* live session number */
	u_int64_t	nowlive;		/* live session number */

	/* the flow information */
	u_int64_t	nclirecv;	/* recv client bytes */
	u_int64_t	nclisend;	/* send client bytes */
	u_int64_t	nsvrrecv;	/* recv server bytes */
	u_int64_t	nsvrsend;	/* send server bytes */

	/* the close/error information */
	u_int64_t	ncliclose;	/* client close times */
	u_int64_t	nclierror;	/* client error times */
	u_int64_t	nsvrclose;	/* server close times */
	u_int64_t	nsvrerror;	/* server error times */
	pthread_mutex_t	lock;		/* lock for proxy stat */
} proxy_stat_t;


typedef struct _db_log_cfg_ {
	char db_ip[32];
	char db_name[32];
	char db_user[32];
	char db_passwd[32];
}db_log_cfg_t;

/**
 *	global proxy struct
 */
typedef struct context {

	/* the proxy address and real server address */
	ip_port_t	httpaddr;	/* HTTP server address */
	ip_port_t	httpsaddr;	/* HTTPS server address */
	ip_port_t	rsaddr;/* real server address */
	int			nrsaddr;	/* real server number */

	/* the thread infomation */
	int			nwork;		/* number of work thread */
	thread_t	works[MAX_WORKNUM];/* the parse thread */
	thread_t	accept;		/* the accept thread */
	thread_t	clock;		/* the accept thread */

	/* the global resource for proxy */
	mempool_t	*pktpool;	/* pool for packet_t */
	mempool_t	*ssnpool;	/* pool for session_t */
	
	u_int32_t	max;		/* max concurrent session*/
	int			stop;		/* proxy stopped */
	int			dbglvl;		/* debug level */
	int			wafid;		/* debug level */
	pthread_t	main_tid;	/* main thread id */
	proxy_stat_t	stat;		/* statistic data */
	pthread_mutex_t	lock;		/* lock for proxy stat */
	pthread_mutex_t	log_lock;		/* lock for log */
	pthread_rwlock_t  cfglock;
	db_log_cfg_t  dcfg;
} context_t;

extern  context_t		g_cfg;


/**
 *	Run a TCP proxy on @ip:@port, the real server is @rip:@rport.
 *	The proxy can process @capacity concurrency connections, the
 *	work thread number is @nworks. If @use_splice is 1, using
 *	splice() to improve performance.
 *
 *	Return 0 if success, -1 on error.
 */

extern int 
proxy_main(ip_port_t *svraddr, int max, int nwork);
extern int 

proxy_main2();

/**
 *	lock proxy statistic data.
 *
 *	No return.
 */
extern void 
proxy_stat_lock();

/**
 *	Unlock proxy statistic data.
 *
 *	No return.
 */
extern void 
proxy_stat_unlock();

extern void
proxy_log_lock();

extern void
proxy_log_unlock();

extern int load_cfg();


#endif /* end of FZ_PROXY_H */


