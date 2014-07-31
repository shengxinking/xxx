#include <stdio.h>

#include "cfg_db.h"
#include "common.h"


cfg_vps_t *cfg_vps = NULL;

extern MYSQL *m_db_cfg ;

MYSQL* Db_Con_cfg()
{
    //const char *host="192.168.135.67", *user="root";
    const char *host="192.168.135.67", *user="root";
    char *pass="";
	char *database = "vps_record";
    unsigned int port = 3306 ;
	MYSQL *db_conn;

	db_conn = mysql_init(NULL);
	if (NULL == db_conn) {
		printf("error : mysql init error \n");
		return NULL;
	}
	if (!mysql_real_connect(db_conn, host, user, pass, database, port, NULL, 0)) {
		printf("error: connect db error \n");
		return NULL;	
	}
	
	if (!cfg_vps) {
		cfg_vps = malloc(sizeof(cfg_vps_t));
		if (!cfg_vps) {
			printf("init CFG_VPS Head node error \n");
			return NULL;
		}
	
	}

	return db_conn;
}
/*caution: the record need escape*/

/* table name: update, rid = record id , act: 0=add|1=update|2=del*/

int  _read_update(char* table_name, MYSQL_RES **res)
{
	char sql[512];
	snprintf(sql, sizeof(sql),"select recid, act from %s",table_name);
	if (m_db_cfg != NULL) {
		 if (mysql_query(m_db_cfg, sql)) {
            printf("%s\n", mysql_error(m_db_cfg));
			return -1;
        }else {
			*res = mysql_store_result(m_db_cfg);
			return 0;
		}
	}
	
	return -2;

}


int _update_vps_node(int rid, int act)
{
	MYSQL_ROW row;
	MYSQL_RES *res;
	
	char sql[512];
	char *table_name = "vps";
	
	cfg_vps_t *p_tmp = NULL, *p_tmp2 = NULL;
	
	snprintf(sql, sizeof(sql),"select vpsid, ip, action from %s", table_name);
	
	if (m_db_cfg != NULL) {
		 if(mysql_query(m_db_cfg, sql)) {
            printf("%s\n", mysql_error(m_db_cfg));
			return -1;
        }else {
			res = mysql_store_result(m_db_cfg);
		}
	}
	
	row = mysql_fetch_row(res);
	if(row == NULL) {
		printf("fetch vps node cfg error \n");
		return -1;
	}
	
	switch (act) {
		case 0:  //add
			p_tmp = malloc(sizeof (cfg_vps));
			if (p_tmp == NULL) {
				printf("malloc failed in cfg_vps\n");
				return -1;
			}	 
			p_tmp->next = cfg_vps->next;
			cfg_vps->next = p_tmp;
			break;
		case 1:  //update
			p_tmp = cfg_vps->next;
			while (p_tmp != NULL) {
				if (p_tmp->vpsid == *(row[0])) {
				}
				p_tmp = p_tmp->next;
			}
			break;
		case 2:  //del
			p_tmp = cfg_vps;
			while (p_tmp != NULL) {
				p_tmp2 = p_tmp->next;
				if (p_tmp2->vpsid == *(row[0])) {
					p_tmp->next = p_tmp2->next;
					free(p_tmp2);
				}
				p_tmp = p_tmp->next;
			}
			break;
		default:
			return -1;
	}
	
	mysql_free_result(res);	

	return 0;	
}

int check_cfg_update()
{
	MYSQL_RES *res;
	MYSQL_ROW row;
		
	unsigned int num_row;

	_read_update("update_rec", &res);	
	num_row = mysql_num_rows(res);
	if (num_row <= 0) {
		DBGC("no record update \n");
		return 0;
	}

	while ((row = mysql_fetch_row(res))) {
		DBGC("update rec = %s:%s\n", (row[0]), (row[1]));	
		_update_vps_node(atoi(row[0]), atoi(row[1]));	
	}

	mysql_free_result(res);	
	return 0;
}


