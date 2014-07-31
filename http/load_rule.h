#ifndef _LOAD_RULE_H
#define _LOAD_RULE_H
#include "pcre.h"
#include "common.h"

#include "common.h"
#include "connection.h"

#define T_URL 		0
#define T_AEG 		1
#define T_AGENT 	2
#define T_COOKIE 	3

#define GET_BIT(x,n) (x&(0x01<<n))
#define SET_BIT(x,n) (x=x|(0x01<<n))

#define SET_URL_TARGET(flag)  SET_BIT(flag,0)
#define GET_URL_TARGET(flag)  GET_BIT(flag,0)

#define SET_ARG_TARGET(flag)  SET_BIT(flag,1)
#define GET_ARG_TARGET(flag)  GET_BIT(flag,1)


#define SET_AGENT_TARGET(flag)  SET_BIT(flag,2)
#define GET_AGENT_TARGET(flag)  GET_BIT(flag,2)


#define SET_COOKIE_TARGET(flag)  SET_BIT(flag,3)
#define GET_COOKIE_TARGET(flag)  GET_BIT(flag,3)

typedef struct rule_node_t {
	struct rule_node_t *next;
	char ruleid[MAX_TXT_LEN2];
	char target[MAX_TXT_LEN2];
	int  i_tar;
	char catalog[MAX_TXT_LEN2];
	int  key;
	char chain[MAX_TXT_LEN2];
	char text[MAX_TXT_LEN];
	pcre *pcre_exp;
	pcre_extra *pcre_extra;
	
	int disable;
} rule_node;


int load_rule();

int rule_check_entry(http_t *p_http, connection_t *conn);

#endif
