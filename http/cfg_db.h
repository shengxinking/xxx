#ifndef A_CFG_DB_H
#define A_CFG_DB_H

#include <my_global.h>
#include <mysql.h>

typedef struct _conf_vps_{
	int vpsid; 
	int ip;		//vps local ip ;
	int action; //alert or deny
	struct _conf_vps_ *next;
}cfg_vps_t;

extern MYSQL* Db_Con_cfg();


extern int check_cfg_update();




#endif
