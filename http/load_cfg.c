#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "proxy.h"



#define CFG_FILE  "waf.cfg"
#define MAX_BUFFER_SIZE  128

extern context_t g_cfg;


int _get_waf_thread(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "WORK_THREAD") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("waf_id = %s\n",p);
		g_cfg.nwork = atoi(p); 
		break;

	}

return 0 ;
}



int _get_waf_id(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "WAF_ID") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("waf_id = %s\n",p);
		g_cfg.wafid = atoi(p); 
		break;

	}

return 0 ;
}





int _get_real_ip(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "REAL_IP") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("real server ip = %s\n",p);
		if (ip_port_from_str(&g_cfg.rsaddr, p)) {
			printf("invalid real ip addresss\n");
			return -1;
		}
		break;

	}

return 0 ;
}


int _get_listen_ip(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "LISTEN_IP") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("listen ip = %s\n",p);
		if (ip_port_from_str(&g_cfg.httpaddr, p)) {
			printf("invalid listen ip addresss\n");
			return -1;
		}
		break;

	}

return 0;
}

int _get_db_passwd(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "DB_PASSWD") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("db passwd = %s\n",p);
		strncpy(g_cfg.dcfg.db_passwd, p, sizeof(g_cfg.dcfg.db_passwd)); 
		*strrchr(g_cfg.dcfg.db_passwd, '\n') = '\0';
printf("len = %ld\n",strlen(g_cfg.dcfg.db_passwd));
		break;

	}

return 0 ;
}

int _get_db_user(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "DB_USER") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("db user = %s\n",p);
		strncpy(g_cfg.dcfg.db_user, p, sizeof(g_cfg.dcfg.db_user)); 
		*strrchr(g_cfg.dcfg.db_user, '\n') = '\0';
		break;

	}

return 0 ;
}

int _get_db_name(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "DB_NAME") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("db name = %s\n",p);
		strncpy(g_cfg.dcfg.db_name, p, sizeof(g_cfg.dcfg.db_name)); 
		*strrchr(g_cfg.dcfg.db_name, '\n') = '\0';
		break;

	}

return 0 ;
}

int _get_db_ip(FILE* fp)
{
	char *p = NULL;
	char buf[MAX_BUFFER_SIZE] = {0};
	
	rewind(fp);	
	
	while ((fgets(buf, MAX_BUFFER_SIZE, fp))) {
	//	DBGF("buf = %s\n",buf);
		if (buf[0] == '#') {
			continue;
		}
		if(strstr(buf, "DB_IP") == NULL) {
			continue;
		}
		p = strtok(buf, "\t");
			if (p == NULL) {
				DBGF(" format error, buf:[%s]\n", buf);
				return -1;
		}
		
		p = strtok(NULL, "\t");
		DBGF("real db ip = %s\n",p);
	
		strncpy(g_cfg.dcfg.db_ip, p, sizeof(g_cfg.dcfg.db_ip)); 
		*strrchr(g_cfg.dcfg.db_ip, '\n') = '\0';
	
		break;

	}

return 0 ;
}

int load_cfg()
{
	int ret = 0;
	FILE *fp;
	fp = fopen(CFG_FILE, "r");
	if (fp == NULL) {
		DBGF("open cfg file error!\n");
		return -1;
	}

	ret = _get_listen_ip(fp);
	if (ret) {
		printf("get listen ip error \n");
		return -1;
	}
	ret = _get_real_ip(fp);
	if (ret){
		printf("get real ip error \n");
		return -1;
	}
	ret = _get_db_passwd(fp);
	if (ret) {
		printf("get db passwd error \n");
		return -1;
	}
	ret =  _get_db_user(fp);
	if (ret) {
		printf("get db user error \n");
		return -1;
	}
	ret = _get_db_name(fp);
	if (ret) {
		printf("get db name error \n");
		return -1;
	}
	ret =  _get_db_ip(fp);
	if (ret) {
		printf("get db ip error \n");
		return -1;
	}
	ret =  _get_waf_id(fp);
	if (ret) {
		printf("get waf id error \n");
		return -1;
	}

	ret =  _get_waf_thread(fp);
	if (ret) {
		printf("get waf id error \n");
		return -1;
	}
	return 0;


}
