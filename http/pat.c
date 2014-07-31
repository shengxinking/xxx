#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/file.h>
#include <errno.h>

#include "pcre_func.h"
#include "connection.h"

#include "load_key.h"
#include "load_rule.h"
#include "http_api.h"

#include "http.h"
#include "http_basic.h"

#include "log_db.h"

#define MAGIC 20

extern ACSM_STRUCT *ac_req;
extern ACSM_STRUCT *ac_resp;
extern rule_node *node;


int _boundary_char_check(char ch);
int _boundary_check(char *name, char *buf, int buf_len, int index, int flag);
int word_boundary_check(char *name, char *buf, int buf_len, int index, int flag);

#define HEX_FLAG "HEX-MATCH" 
#define HEX_LEN 4
#define BUF_LEN 5
#define MSG_IDX 8
#define SUB_IDX 15

enum{
        CHECK_NONE = 0,
        CHECK_LEFT ,
        CHECK_RIGHT,
        CHECK_BOTH,
};


typedef struct _key_data_t {
	char *text;
	int  len;
	unsigned int target;
	int cnt;
	int index[20];
} key_data;

char  attack_msg[MSG_IDX][32] = {
	{"XSS"},
	{"SQL"},
	{"trojan"},
	{"general_attack"},
	{"information_leakage"},
	{"id_card"},
	{"social_security_cards"},

};

char sub_msg[SUB_IDX][32] = {
	{"OS 命令注入"},
	{"Coldfusion 注入"},
	{"LDAP"},
	{"命令注入"},
	{"Session会话攻击"},
	{"File注入"},
	{"PHP注入"},
	{"SSI注入"},
	{"UPDF-xss"},
	{"Email注入"},
	{"HTTP回应分割"},
	{"RFI注入"},
	{"LFI注入"},
	{"SRC泄漏"},
};

int check_hex_encode(char *src, char *dst)
{
	char *p_tmp = NULL;
	char *p_begin = NULL;
	char *p_end = NULL;
	char tmp[BUF_LEN] = {0};
	int i = 0;

	if(strstr(src, HEX_FLAG) !=NULL){
		p_begin = p_end = src;
		while((p_end = strchr(p_begin,','))){
			p_tmp = strstr(p_begin,"0X");
			if(p_tmp != NULL){
				memcpy(tmp, p_tmp , HEX_LEN);
				tmp[BUF_LEN-1] = '\0';
				dst[i] = strtoul(tmp , 0 , 16);
				i++; 
			}
			p_begin = p_end + 1;
		}
		p_tmp = strstr(p_begin,"0X");
		if(p_tmp != NULL){
			memcpy(tmp, p_tmp , HEX_LEN);
			tmp[BUF_LEN-1] = '\0';
			dst[i] = strtoul(tmp , 0 , 16);
			i++; 
		}
		
		dst[i] = '\0';
		return i;
	}
	return 0;
}


int match_hook(void * id, int offset, void *data)
{
	int ret = 0;
	int i = 0;

	key_node_t *p_node;
	key_data *p_data;

	p_node = (key_node_t *)id;
	p_data = (key_data *)data;

	ret = word_boundary_check((char *)p_node->txt, p_data->text, \
					  p_data->len, offset, p_node->flag);

	if (ret == 1) {
		DBGR("find key: %s %d\n", p_node->txt, p_node->id);
			/* match */
		if (p_data->cnt >=20) {
			return 0;	
		}
		i = p_data->cnt;
		p_data->index[i] = p_node->id;
		p_data->cnt++;
		// index limit 20
	}

	return 0;
}

int search_multi_pattern(ACSM_STRUCT *acbm_root, key_data *p_data, char *text, int len)
{
	int start_state = 0;

	if (text == NULL || len <= 0) {
		return 0;
	}

	acsmSearch(acbm_root, (unsigned char *)text, len, match_hook, p_data, &start_state);

	return 0;
}

int _boundary_char_check(char ch)
{
        if ((ch >= '0' && ch <= '9')
                        || (ch >= 'a' && ch <= 'z')
                        || (ch >= 'A' && ch <= 'Z')
                        || (ch == '_')
			|| (ch == '-')
			) {
                return 0;
        }
        return 1;
}

int _boundary_check(char *name, char *buf, int buf_len, int index, int flag)
{
        int ret = 0;

        if (flag == CHECK_LEFT) {
                if (index == 0) {
                        ret = 1;
                        goto end;
                }
                ret = _boundary_char_check(buf[index-1]);
        }
        else if (flag == CHECK_RIGHT) {
                if (index+strlen(name) == buf_len) {
                        ret = 1;
                        goto end;
                }
                ret = _boundary_char_check(buf[index+strlen(name)]);
        }

end:
        return ret;
}

#define WHITE_ZH_CN "zh-"

/* 
 *	Return 1: It is like an attack.
 *             0: No attack.
 * 
 * */
int word_boundary_check(char *name, char *buf, int buf_len, int index, int flag)
{
        int ret = 0;

        if (flag == CHECK_NONE)
                return 1;

        if (!name || !buf)
                return ret;
	
	if (buf_len <= 0)
		return ret;

        if (index >= buf_len)
                return ret;

        if (strlen(name) > buf_len-index)
                return ret;

        switch (flag) {
                case CHECK_LEFT:
                case CHECK_RIGHT:
                        ret = _boundary_check(name, buf, buf_len, index, flag);
                        break;
                case CHECK_BOTH:
                        if (_boundary_check(name, buf, buf_len, index, 1) == 1) {
                                if (_boundary_check(name, buf, buf_len, index, 2) == 1)
                                        ret = 1;
                        }
                        break;
                default:
                        break;
        }

        return ret;
}

#if 0
static int log_others(url_access_t *p)
{
	g_action = p->action;
	g_log_info.severity = p->level; 
	g_log_info.type = MD_URL_LIMIT; 
	
	return 0;
}
#endif

int rule_check_arg(http_t *p_http, connection_t *conn)
{
	//char txt[2048] = {0} ;
	int i = 0,j = 0;
	int id = 0;
	int begin = 0, end = 0;
	int ret = -1;
	
	sublog_t slog;
	memset(&slog, 0, sizeof(sublog_t));
	
	rule_node *p_node = NULL;

	key_data my_data ;
	key_data *p_data = &my_data;
	memset(p_data, 0, sizeof(key_data));

	http_arg_ctrl_t *ctl = http_get_arg_t(p_http);
	http_arg_t *arg = NULL;
	int num = ctl->num;
	
	for ( j = 0; j < num; j++) {
		arg = &ctl->arg[j];
		p_data->cnt = 0;
		p_data->text = arg->value.p;
		p_data->len = arg->value.len;
		if(ac_req == NULL){
			printf("error return--\n");
		}
		DBGR("AGR = %s\n",p_data->text);
	
		search_multi_pattern(ac_req, p_data, p_data->text, p_data->len);
	
		for (i = 0 ; i < p_data->cnt; i++){
			DBGR("Key_id = %d\n",p_data->index[i]);
			id = p_data->index[i];
		
			/*caution: need modify, one id many rule*/	
			p_node = &node[id];
			while (p_node != NULL) {
				if (p_node->disable == 1||GET_ARG_TARGET(p_node->i_tar) == 0) {
					p_node = p_node->next;
					continue;
				}
				DBGR("regexp= %s\n",p_node->text);	
				ret = pcre_match_exp(p_node->pcre_exp, p_node->pcre_extra,\
								    p_data->text, p_data->len, &begin, &end);
				if (ret > 0) {
					strncpy(slog.id, p_node->ruleid, sizeof(slog.id));
					strncpy(slog.catalog, p_node->catalog, sizeof(slog.catalog));
					log_attack(conn, &slog);
					return RET_FAIL;
				}
				p_node = p_node->next;
			}
		}	
	}
	

	return RET_OK;

}
int rule_check_url(http_t *p_http, connection_t *conn)
{
	//char txt[2048] = {0} ;
	int i = 0;
	int id = 0;
	int begin = 0, end = 0;
	int ret = -1;

	sublog_t slog;
	memset(&slog, 0, sizeof(sublog_t));
	
	rule_node *p_node = NULL;
	
	key_data my_data  ;
	key_data *p_data = &my_data;
	memset(p_data, 0, sizeof(key_data));

	p_data->cnt = 0;
	p_data->text = http_get_str(p_http,HTTP_STR_URL);
	p_data->len = http_get_str_len(p_http,HTTP_STR_URL);

	if(ac_req == NULL){
		printf("error return--\n");
	}
	DBGR("url = %s\n",p_data->text);
	search_multi_pattern(ac_req, p_data, p_data->text, p_data->len);
	for(i = 0 ; i < p_data->cnt; i++){
		DBGR("key_id = %d\n",p_data->index[i]);
		id = p_data->index[i];
			/*caution: need modify, one id many rule*/	
			p_node = &node[id];
			while (p_node != NULL) {
				if (p_node->disable == 1||GET_ARG_TARGET(p_node->i_tar) == 0) {
					p_node = p_node->next;
					continue;
				}
				DBGR("regexp= %s\n",p_node->text);	
				ret = pcre_match_exp(p_node->pcre_exp, p_node->pcre_extra,\
								    p_data->text, p_data->len, &begin, &end);
				if (ret > 0) {
					strncpy(slog.id, p_node->ruleid, sizeof(slog.id));
					strncpy(slog.catalog, p_node->catalog, sizeof(slog.catalog));
					log_attack(conn, &slog);
					return RET_FAIL;
				}
				p_node = p_node->next;
			}
	}
	
	return RET_OK;
}

int rule_check_header_cookie(http_t *p_http, connection_t *conn)
{
	//char txt[2048] = {0} ;
	int i = 0;
	int id = 0;
	int begin = 0, end = 0;
	int ret = -1;
	
	sublog_t slog;
	memset(&slog, 0, sizeof(sublog_t));
	
	rule_node *p_node = NULL;

	key_data my_data  ;
	key_data *p_data = &my_data;
	memset(p_data, 0, sizeof(key_data));
	p_data->cnt = 0;
	p_data->text = http_get_str(p_http,HTTP_STR_COOKIE);
	p_data->len = http_get_str_len(p_http,HTTP_STR_COOKIE);
	if (ac_req == NULL){
		printf("error return--\n");
	}
	DBGR("cookie = %s\n",p_data->text);
	search_multi_pattern(ac_req, p_data, p_data->text, p_data->len);
	for (i = 0 ; i < p_data->cnt; i++){
		DBGR("key_id = %d\n",p_data->index[i]);
		id = p_data->index[i];
		p_node = &node[id];
			while (p_node != NULL) {
				if (p_node->disable == 1||GET_ARG_TARGET(p_node->i_tar) == 0) {
					p_node = p_node->next;
					continue;
				}
				DBGR("regexp= %s\n",p_node->text);	
				ret = pcre_match_exp(p_node->pcre_exp, p_node->pcre_extra,\
								    p_data->text, p_data->len, &begin, &end);
				if (ret > 0) {
					strncpy(slog.id, p_node->ruleid, sizeof(slog.id));
					strncpy(slog.catalog, p_node->catalog, sizeof(slog.catalog));
					log_attack(conn, &slog);
					return RET_FAIL;
				}
				p_node = p_node->next;
			}
	}
	
	return RET_OK;

}



int rule_check_header_agent(http_t *p_http, connection_t *conn)
{
	//char txt[2048] = {0} ;
	int i = 0;
	int id = 0;
	int begin = 0, end = 0;
	int ret = -1;

	sublog_t slog;
	memset(&slog, 0, sizeof(sublog_t));
	
	rule_node *p_node = NULL;

	key_data my_data  ;
	key_data *p_data = &my_data;
	memset(p_data, 0, sizeof(key_data));
	p_data->cnt = 0;
	p_data->text = http_get_str(p_http,HTTP_STR_AGENT);
	p_data->len = http_get_str_len(p_http,HTTP_STR_AGENT);
	if(ac_req == NULL){
		printf("error return--\n");
	}
	DBGR("AGENT = %s\n",p_data->text);
	search_multi_pattern(ac_req, p_data, p_data->text, p_data->len);
	for(i = 0 ; i < p_data->cnt; i++){
		DBGR("key_id = %d\n",p_data->index[i]);
		id = p_data->index[i];
		p_node = &node[id];
		
		while (p_node != NULL) {
			if (p_node->disable == 1||GET_ARG_TARGET(p_node->i_tar) == 0) {
					p_node = p_node->next;
					continue;
			}
			DBGR("regexp= %s\n",p_node->text);	
			ret = pcre_match_exp(p_node->pcre_exp, p_node->pcre_extra,\
							    p_data->text, p_data->len, &begin, &end);
			if (ret > 0) {
				strncpy(slog.id, p_node->ruleid, sizeof(slog.id));
				strncpy(slog.catalog, p_node->catalog, sizeof(slog.catalog));
				log_attack(conn, &slog);
				return RET_FAIL;
			}
				p_node = p_node->next;
		}
	}

	return RET_OK;
}


int rule_check(http_t *p_http, connection_t *conn)
{
        int ret = 0;

        ret =  rule_check_url(p_http, conn);
        if (RET_FAIL == ret)
                return ret;
        ret = rule_check_arg(p_http, conn);
        if (RET_FAIL == ret)
                return ret;
        ret = rule_check_header_agent(p_http, conn);
        if (RET_FAIL == ret)
                return ret;
        ret = rule_check_header_cookie(p_http, conn);
        if (RET_FAIL == ret)
                return ret;

        return RET_OK;
}

int rule_check_entry(http_t *p_http, connection_t *conn)
{
	DBGR("enter rule check ===\n");
#if 0
	web_cfg_t *p_cfg = &p_policy->profile.web_cfg;
	if (p_cfg == NULL)
		return 0;
	if(p_cfg->changed) {
		load_web_attack(p_cfg);
	}	
	
#endif
	return rule_check(p_http, conn);
}




#if 0
int rule_test()
{
	char txt[2048] = {0} ;
	int i = 0;
	int id = 0;
	int begin = 0, end = 0;
	int index = 0;
	int ret = -1;

	key_data *p_data = NULL;
	p_data = malloc(sizeof(key_data));
	if(p_data == NULL){
		printf("malloc error---\n");
		return -1;
	}
	p_data->cnt = 0;
//	init_fast_match();
//	load_rule();
	strcpy(txt ,"GET /login?picfilename=<script>alert(xss)</script>  HTTP/1.1");
	p_data->text = txt;
	p_data->len =  strlen(txt);
	if(ac_req == NULL){
		printf("error return--\n");
	}
	search_multi_pattern(ac_req, p_data, txt, p_data->len);
	for(i = 0 ; i < p_data->cnt; i++){
		printf("id = %d\n",p_data->index[i]);
		id = p_data->index[i];
		printf("text = %s\n",p_rule_node[id].text);
		
		/*caution: need modify, one id many rule*/	
		p_node = &node[id];
		if (p_node->disable == 1)
			continue;
			
		while (p_node != NULL) {
			printf("text= %s\n",p_node->text);	
			ret = pcre_match_exp(p_node->pcre_exp, p_node->pcre_extra,\
							    p_data->text, p_data->len, &begin, &end);
		}

		if (ret > 0) {
                	printf("match-----------\n");
		} else {
                	printf("No  match-----------\n");
        	}

	}
	
//	free(p_data);

	return 0;
}

#endif
