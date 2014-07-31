#ifndef A_LOG_H
#define A_LOG_H

#include <sys/types.h>
#include <time.h>
#include <my_global.h>
#include <mysql.h>
#include "common.h"
#include "connection.h"

#define PKT_LEN 4096

typedef struct _alog_{
        time_t  time;
        char 	host[M_LEN];
        char 	url[M_LEN-1];

        char 	src_ip[24];
        char 	dst_ip[24];
        int 	src_port;
        int 	dst_port;

        char    id[MAX_TXT_LEN2]; //规则id
    	char    catalog[MAX_TXT_LEN2];    
        int 	action;
        char 	method[10];

        char 	msg[M_LEN];
        char 	agent[M_LEN];
		char 	cookie[L_LEN];
        char 	raw[2047];
}alog_t;

typedef struct _t_log_t{
	u_int64_t clirecv;
	u_int64_t svrrecv;
	u_int64_t nlive;
	u_int64_t nowlive;
}tlog_t;


typedef struct _sub_log_{
	char id[MAX_TXT_LEN2];
	char catalog[MAX_TXT_LEN2];
}sublog_t;

extern int log_attack(connection_t *conn, sublog_t *slog );
extern int log_traffic(tlog_t *tlog );

extern MYSQL* Db_Con_log();

#endif
