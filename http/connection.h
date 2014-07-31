#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "packet.h"
#include "ip_addr.h"
#include "http.h"

/**
 *	The connection packet queue.
 */
typedef struct connection_queue {
	pktq_t		inq;		/* the input queue */
	pktq_t		outq;		/* the output queue */
	packet_t	*pkt;		/* the current queue */
	packet_t	*blkpkt;	/* blocked packet */
	u_int32_t	pos;		/* pos of next recv byte */
	u_int32_t	nbyte;		/* number of bytes in queue */
} connection_queue_t;

/**
 *	connection structure.
 */
typedef struct connection {
        
	int		id;		/* connection id */
	u_int8_t	index;		/* index of work thread */
	u_int32_t	magic;		/* connection magic number */

//	ip_port_t	cliaddr;	/* the client address */
	ip_addr_t       cliaddr;        /* client address */
	u_int16_t       cliport;        /* client port */
	int		clifd;		/* client socket */
	connection_queue_t	cliq;		/* client packet queue */
//	ep_event_t	cliev;		/* client read event */

//	ip_port_t	svraddr;	/* the server address */
	ip_addr_t       svraddr;        /* the server address */
	u_int16_t       svrport;
	int		svrfd;		/* server socket */
	connection_queue_t	svrq;		/* server packet queue */
//	ep_event_t	svrev;		/* server read event */
	
	int		read_pipe;	/* read pipe for splice */
	int		write_pipe;	/* write pipe for splice */

	int		nalloced;	/* alloced packet number */

	/* connection flags */
	u_int32_t	is_clierror:1;	/* client error */
	u_int32_t	is_cliclose:1;	/* client closed */
	u_int32_t	is_clisslwait:1;/* client need wait SSL */
	u_int32_t	is_cliblock:1;	/* client side is blocked*/
	
	u_int32_t	is_svrerror:1;	/* server error */
	u_int32_t	is_svrclose:1;	/* server closed */
	u_int32_t	is_svrwait:1;	/* wait TCP connection */
	u_int32_t	is_svrsslwait:1;/* server wait SSL */
	u_int32_t	is_svrblock:1;	/* server is blocked */
	
	http_t		http_ctx;
} connection_t;

/**
 * 	The connection tup, using in offline hash find.
 */
typedef struct connection_tup {
	int		id;		/* connection id for inline */
	ip_port_t	svraddr;	/* server address */
	ip_port_t	cliaddr;	/* client address */
	int		clifd;		/* the client fd */
	int		svrfd;		/* the server fd */
	u_int16_t	index;		/* the index for offline */
} connection_tup_t;

/**
 *	connection table
 */
typedef struct connection_table {
	connection_t	**table;	/* the connection table */
	size_t		max;		/* the max table size */
	size_t		nfreed;		/* number of freed entry */
	int		freeid;		/* the freed index */
	pthread_mutex_t	lock;		/* lock for table */
} connection_table_t;

/**
 * 	Compair 2 connection_tup_t object is same or not.
 *	Using in hash table. 
 *	if clifd > 0, it's inline mode and compare id,
 *	or else it's offline mode and compre sip:sport->dip:dport.
 *	
 *	Return 0 if equal, !=0 if not equal;
 */
extern int 
connection_tup_compare(connection_tup_t *tup1, connection_tup_t *tup2);

/**
 *	Alloc a connection table and return it.
 *
 *	Return object ptr if success, NULL on error.
 */
extern connection_table_t * 
connection_table_alloc(size_t max);

/**
 *	Free connection map @m.
 *
 *	No return.
 */
extern void 
connection_table_free(connection_table_t *st);

/**
 *	Lock connection map @m.
 *
 *	No return.
 */
extern void 
connection_table_lock(connection_table_t *st);

/**
 *	Unlock connection map @m.
 *
 *	No return.
 */
extern void 
connection_table_unlock(connection_table_t *st);

/**
 *	Add new connection into connection table 
 *
 *	Return the 0 if success, -1 on error.
 */
extern int 
connection_table_add(connection_table_t *st, connection_t *s);

/**
 *	Delete connection which id=@id from connection table @st.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
connection_table_del(connection_table_t *st, int id);

/**
 * 	Find a connection from connection table @st acording
 *	connection's ID.
 *
 *	Return pointer if success, NULL on error.
 */
extern connection_t *  
connection_table_find(connection_table_t *st, int id);

/**
 *	Print the whole connection table
 *
 *	No return.
 */
extern void 
connection_table_print(connection_table_t *st);

/**
 *	Print the connection information.
 *
 *	No return.
 */
extern void 
connection_print(const connection_t *s);

extern char* get_pkt_buf(connection_t *conn, int *len, int dir);
#endif /* end of FZ_connection_H  */

