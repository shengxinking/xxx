#include "log_db.h"
#include "common.h"
#include "http_api.h"
#include "proxy.h"


char method[HTTP_METHOD_NUM][10]= {
"NONE",
"NA",
"GET",
"POST",
"HEAD",
"OPTION",
"PUT",
"CONNECT",
"DELETE",
"TRACE",
"WEBDAV",
};

extern MYSQL *m_db_log ;

static int _escape_append_str( char *des, const char *str, int max )
{
        int n = 0;

        if ( str == NULL )
        {
                return 0;
        }
        for ( n = 0; *str != '\0'&& n < max; str++, n++ )
        {
                if ( *str == '\"')
                {
                        des[n] = '\\';
                        n++;
                        des[n] = '\"';
                } else
                {
                        des[n] = *str;
                }
        }
        return n;
}


MYSQL* Db_Con_log()
{
//    const char *host="192.168.135.67", *user="root";
//  char *pass="";
//	const char *host="10.143.1.248", *user="waf_log";
//	char *pass="he1md@ll";
//	char *database = "waf";

    unsigned int port = 3306 ;
	MYSQL *db_conn;

	db_conn = mysql_init(NULL);
	if (NULL == db_conn) {
		printf("error : mysql init error \n");
		return NULL;
	}
#if 0
	if (!mysql_real_connect(db_conn, "192.168.135.67", "waf_log", "he1md@11","waf", port, NULL, 0)) {
		printf("%s\n", mysql_error(db_conn));
		return NULL;	
	}
#endif
#if 1
	printf("db:%s:%s:%s:%s\n",g_cfg.dcfg.db_ip, g_cfg.dcfg.db_user,g_cfg.dcfg.db_passwd, g_cfg.dcfg.db_name);

	if (!mysql_real_connect(db_conn, g_cfg.dcfg.db_ip, g_cfg.dcfg.db_user,\
							g_cfg.dcfg.db_passwd, g_cfg.dcfg.db_name, port, NULL, 0)) {
		printf("%s\n", mysql_error(db_conn));
		return NULL;	
	}
#endif
	return db_conn;
}

void Db_close(MYSQL **db_conn) {
	mysql_close(*db_conn);
}

int recon_log_db ()
{
	printf(" reconnect log db \n");
	Db_close(&m_db_log); 
	m_db_log = Db_Con_log();
	return 0;
}


int log_traffic(tlog_t *tlog)
{
	char sql[1024];
	snprintf(sql,sizeof(sql),"insert into waf_status_cdate(agent,cli_recv,svr_recv,"
							  "new_conn, now_conn)"
							 "values(%d,%ld,%ld,%ld,%ld)",	
						     g_cfg.wafid, tlog->clirecv, tlog->svrrecv,
							 tlog->nlive, tlog->nowlive);
	DBGL("%s\n",sql);
	if (m_db_log != NULL) {
		if (mysql_query(m_db_log, sql)) {
			printf("%s\n", mysql_error(m_db_log));
			recon_log_db();			
			if (mysql_query(m_db_log, sql)) {
				printf("caution :log db is error \n ");
				return -1;
			}
		}
	}
	
	return 0;
}


int log_data(MYSQL **db_conn, alog_t *log)
{
	char sql[4096];
	snprintf(sql,sizeof(sql),"insert into waf_log_cdate(time,host,url,"
				"src_ip,dst_ip,src_port,dst_port,"
				"sig_id, catalog,action,method,"
				"msg,agent,cookie,raw) "
                "values(%d,\"%s\",\"%s\","
                "\"%s\",\"%s\",%d, %d,"
                "\"%s\",\"%s\",%d,\"%s\","
                "\"%s\",\"%s\",\"%s\",\"%s\")",
                        (int)log->time,log->host,log->url,
                        log->src_ip,log->dst_ip,log->src_port,log->dst_port,
                        log->id,log->catalog,log->action,log->method,
                        log->msg,log->agent,log->cookie,log->raw);
	DBGL("%s\n",sql);
	if (*db_conn != NULL) {
		if (mysql_query(*db_conn, sql)) {
			printf("%s\n", mysql_error(*db_conn));
			recon_log_db();			
			if (mysql_query(*db_conn, sql)) {
				printf("caution :log db is error \n ");
				return -1;
			}
		}
	}	

	return 0;
}



int log_attack(connection_t *conn, sublog_t *slog )
{
        char buf[32];
        char *p_url, *p_host, *p_pkt, *p_cookie, *p_agent;
    	int len, index, n; 	

		if (!conn || !slog) {
			return -1;
		}

	
		alog_t alog;
		memset(&alog,0x00,sizeof(alog_t));
	
		http_t *p_http = &conn->http_ctx;       
 
		alog.time=time(0);
	
		p_host = http_get_str(p_http, HTTP_STR_HOST);
        strncpy(alog.host,p_host,sizeof(alog.host));
        
        p_url = http_get_str(p_http, HTTP_STR_URL);
        //strncpy(alog.url,p_url,sizeof(alog.url));
		_escape_append_str(alog.url, p_url, sizeof(alog.url)-1);
		
		index = http_get_logic(p_http,HTTP_LOGIC_METHOD);
		strncpy(alog.method, method[index], sizeof(alog.method));	

        ip_addr_to_str(&conn->cliaddr, buf, sizeof(buf));
        strncpy(alog.src_ip,buf,sizeof(alog.src_ip));
        
		alog.src_port = conn->cliport;

//		ip_addr_to_str(&g_cfg.httpaddr, buf, sizeof(buf));
  //      strncpy(alog.dst_ip,buf,sizeof(alog.dst_ip));
		
//		log.dst_port =ntohs(_g_svraddr.port);
//      printf("src ip= %s\n",buf);

//      DBGM("host= %s\n",p_host);


        strncpy(alog.id, slog->id, sizeof(slog->id));
        
		strncpy(alog.catalog, slog->catalog,sizeof(slog->catalog));

		p_agent = http_get_str(p_http,HTTP_STR_AGENT);
		n = _escape_append_str(alog.agent, p_agent, sizeof(alog.agent)-1);

		p_cookie = http_get_str(p_http,HTTP_STR_COOKIE);
//		strncpy(alog.cookie, p_cookie, sizeof(alog.cookie)-1);	
		_escape_append_str(alog.cookie, p_cookie, sizeof(alog.cookie));


        p_pkt = get_pkt_buf(conn, &len, 0);
		//printf("len = %d\n",len);
        if (p_pkt) {
			if (len >= sizeof(alog.raw)) {
				len = sizeof(alog.raw) - 1;
			}
			_escape_append_str(alog.raw, p_pkt, len);
        }

	
	proxy_log_lock();
	log_data(&m_db_log, &alog);
	proxy_log_unlock();
	return 0;
}


