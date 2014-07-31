#include <stdio.h>
#include <string.h>
#include "load_key.h"
#include "sqlite_db.h"
#include<stdlib.h>
#include "common.h"
#define size 1000

typedef struct _pat_t {
        char text[MAX_TXT_LEN];
        int  id;
	int  flag;
}__attribute__((packed)) pat_t;

key_node_t *p_key_node;
ACSM_STRUCT *ac_req;
ACSM_STRUCT *ac_resp;

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

int search_key( waf_signature_search_object *obj)
{
	int ret = 0;
        char sql[ MAX_SQL_STR_LEN ] = { 0 };
	if(  signature_check_error( obj ) )
        {
                printf("search_signature_top,invalid data object");
                goto fail;
        }
	
	snprintf(sql,MAX_SQL_STR_LEN,"select key_word, key_word_id, border_check from key_word ");
	ret = obj -> add_field( obj, DB_TYPE_STRING, MAX_TXT_LEN );
	ret = obj -> add_field( obj, DB_TYPE_INT, 0 );
	ret = obj -> add_field( obj, DB_TYPE_INT, 0 );
	obj -> process_query( obj,sql,&ret );
	if( obj -> record_count <= 0 ){
		printf("Top signature record count = 0\n ");
		goto fail;
	}

	return 0;
	
fail:
	return -1;

}

int load_key()
{
	int ret = 0;
	int k_size = 0;
	int i = 0;
	pat_t *p_key;
	sqlite3 *db;
	data_record_t *record = NULL;


	k_size = 1000 * sizeof(key_node_t);
	p_key_node = malloc(k_size);
	if (!p_key_node) {
		printf("malloc node error---\n");
		return 0;
	}
	memset(p_key_node, 0, size);
	waf_signature_search_object  obj;
	ret = db_open( &db , RULE_DB );
	ret = init_db_object( &obj,db );
	
	ret = search_key(&obj);
	record = obj.record;
	while(record != NULL) {
		p_key = (pat_t*) (record->data);
		strcpy(p_key_node[i].txt, (const char *)p_key->text);
		p_key_node[i].id = p_key->id;
		p_key_node[i].flag = p_key->flag;
		DBGK("key= : %s\n",p_key->text);
		record = record ->next;
		i++ ;
		
	}
	
	return i;
	
}




int init_fast_match()
{
	int i, num, len; 
	num = load_key();
	ac_req = acsmNew();
        if (ac_req == NULL) {
                printf("%s: acsmNew() failed.\n", __FUNCTION__);
                return -1;
        }
	for( i = 0 ; i < num; i++ )
	{
		 len = strlen(p_key_node[i].txt);
		 acsmAddPattern(ac_req, (unsigned char *)(p_key_node[i].txt), \
                                       len, 1, 0, 0, \
                                       (void *)(&p_key_node[i]), i);
	}

	acsmCompile(ac_req);	
	return 0;

}

