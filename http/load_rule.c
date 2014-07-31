#include <stdio.h>
#include <string.h>
#include "load_rule.h"
#include "sqlite_db.h"
#include<stdlib.h>

#include "pcre.h"
#include "pcre_func.h"
#include "common.h"
#define 	RULE_SIZE 1000


typedef struct sec_rule_t {
	char id[MAX_TXT_LEN2];
	char text[MAX_TXT_LEN];
	char catalog[MAX_TXT_LEN2];
	char target[MAX_TXT_LEN2];
	char chain[MAX_TXT_LEN2];
	int key;
}__attribute__((packed)) sec_rule_t;



rule_node *node = NULL;

static int _set_target(char *target, int *i_tar)
{
	int test = 0;
//DBGR("target =%s\n",target);
	if (strstr(target,"URL")) {
		SET_URL_TARGET(*i_tar);	
	}
	if (strstr(target,"ARG")) {
		SET_ARG_TARGET(*i_tar);	
	}
	if (strstr(target,"AGENT")) {
		SET_AGENT_TARGET(*i_tar);	
	}
	if (strstr(target,"COOKIE")) {
		SET_COOKIE_TARGET(*i_tar);	
	}
	
	//test = GET_ARG_TARGET(*i_tar);
	test = GET_COOKIE_TARGET(*i_tar);
//	DBGR("arg target =%d\n",test);
	return 0;
}


static int signature_check_error( waf_signature_search_object *obj )
{
        if( !obj )
                return -1;

        if( obj -> db == NULL )
                return -1;

        if( obj -> fail_record != 0 )
                return -1;

        return 0;
}

int search_rule( waf_signature_search_object *obj)
{
	int ret = 0;
        char sql[ MAX_SQL_STR_LEN ] = { 0 };
	if(  signature_check_error( obj ) )
        {
                printf("search_signature_top,invalid data object");
                goto fail;
        }
	
	snprintf(sql,MAX_SQL_STR_LEN,"select id, rule, catalog, target, chain, keyset  from SecRule ");
	ret = obj -> add_field( obj, DB_TYPE_STRING, MAX_TXT_LEN2 );
	ret = obj -> add_field( obj, DB_TYPE_STRING, MAX_TXT_LEN );
	ret = obj -> add_field( obj, DB_TYPE_STRING, MAX_TXT_LEN2 );
	ret = obj -> add_field( obj, DB_TYPE_STRING, MAX_TXT_LEN2 );
	ret = obj -> add_field( obj, DB_TYPE_STRING, MAX_TXT_LEN2 );
	ret = obj -> add_field( obj, DB_TYPE_INT, 0);
	obj -> process_query( obj,sql,&ret );
	if( obj -> record_count <= 0 ){
		printf("Top signature record count = 0\n ");
		goto fail;
	}

	return 0;
	
fail:
	return -1;

}



int load_rule()
{
	node = malloc (RULE_SIZE * sizeof(rule_node));
	memset (node, 0, RULE_SIZE * sizeof(rule_node));
	sec_rule_t *rule;
	int ret = 0;
	sqlite3 *db;
	int index = -1;
	data_record_t *record = NULL;
	waf_signature_search_object  obj;
	ret = db_open(&db, RULE_DB);
	ret = init_db_object(&obj,db);
	
	ret = search_rule(&obj);
	record = obj.record;
	while(record != NULL) {
		rule = (sec_rule_t*) (record->data);
		index = rule->key;
		if (index < 0) {
			continue;
		}
		if (strlen(node[index].text) == 0){
				node[index].key = index;
				strncpy(node[index].text , rule->text, sizeof(node[index].text));
				strncpy(node[index].ruleid , rule->id, sizeof(node[index].ruleid));
				strncpy(node[index].catalog , rule->catalog, sizeof(node[index].catalog));
				strncpy(node[index].target , rule->target, sizeof(node[index].target));
				_set_target(rule->target,&(node[index].i_tar));
				DBGR("Head:keyid:(%d):regexp:%s\n",index,rule->text);				

				node[index].pcre_exp = pcre_compile_exp(rule->text, PCRE_CASELESS);
				if (node[index].pcre_exp == NULL) {
	                        printf("%s: pcre_compile_expression() failed.\n", __FUNCTION__);
        	                return -1;
                }

                node[index].pcre_extra = pcre_study_exp(node[index].pcre_exp, 0);
               	if (node[index].pcre_extra == NULL) {
                        	printf("%s: pcre_study_expression_new() failed.\n", __FUNCTION__);
                        	return -1;
                }
	
		}else {
			rule_node * p_tmp = malloc(sizeof(rule_node));
			memset(p_tmp, 0, sizeof(rule_node));
			p_tmp->key = index;
			strncpy(p_tmp->text, rule->text, sizeof(p_tmp->text));
			strncpy(p_tmp->ruleid , rule->id, sizeof(p_tmp->ruleid));
			strncpy(p_tmp->catalog , rule->catalog, sizeof(p_tmp->catalog));
			strncpy(p_tmp->target , rule->target, sizeof(node[index].target));
			_set_target(rule->target,&(p_tmp->i_tar));
			DBGR("Follow:keyid:(%d):regexp:%s\n",index,rule->text);				
		
			p_tmp->pcre_exp = pcre_compile_exp(rule->text, PCRE_CASELESS);
			if (p_tmp->pcre_exp == NULL) {
	                        printf("%s: pcre_compile_expression() failed.\n", __FUNCTION__);
        	                return -1;
                	}

            p_tmp->pcre_extra = pcre_study_exp(p_tmp->pcre_exp, 0);
            if (p_tmp->pcre_extra == NULL) {
                        	printf("%s: pcre_study_expression_new() failed.\n", __FUNCTION__);
                        	return -1;
            }
			p_tmp->next = node[index].next;
			node[index].next = p_tmp;
		}	
		record = record ->next;
		
	}
	return 0;
	
}
