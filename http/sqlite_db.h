#ifndef _WAF_SIGNATURE_DB_H
#define _WAF_SIGNATURE_DB_H

//#define DB_PATH "/var/log/waf_signature.db"

#define COLLECTION_FIELDS_LEN 64
#define SUB_CLASS_ID_LEN 9
#define MAIN_CLASS_ID_LEN 9
#define SIGNATURE_ID_LEN 9
#define MAX_KEYWORD_REQUIRED 64
#define MAX_COLLECTION_NUM 32
#define MAX_FILTER_NUM_PER_SIGNATURE 16

#define MEET_TARGET_LEN 1024
#define OPERATOR_LEN 8
#define REGEXP_LEN 1024

#define MAX_SQL_STR_LEN 1024
#define MAX_FIELD_NUMBER 64
#define DATA_LEN_LIMIT 4097

#define ADD_FIELD_SUCCESS 0
#define ADD_FIELD_FAIL 1

#define OPEN_DB_FAIL 21
#define PREPARE_DB_ERROR 22
#define STEP_DB_ERROR 23
#define INVALID_DATA_LENGTH 24
#define INVALID_DATA_TYPE 25
#define ALLOC_RECORD_MEM_FAIL 26
#define ALLOC_DATA_MEM_FAIL 27
#define INT_DATA_LEN_OVERFLOW 28
#define TEXT_DATA_LEN_OVERFLOW 29
#define TEXT_LEN_OVERFLOW 30
#define GET_TEXT_FIELD_FAIL 31



#include "sqlite3.h"

typedef struct data_record_t
{
	void *data;
	struct data_record_t *next;

	//added for chain
	unsigned short chain_used;
}data_record_t;

typedef struct field_info_t
{
	#define DB_TYPE_INT 101
	#define DB_TYPE_STRING 102

	unsigned short data_type;
	unsigned int max_data_len;
}field_info_t;

typedef struct waf_signature_search_object
{
	unsigned short field_list_sz;
	field_info_t field_list[ MAX_FIELD_NUMBER ];
	data_record_t *record;
	unsigned short fail_record;
	unsigned int record_count;
	sqlite3 *db;

	int ( *add_field )( struct waf_signature_search_object *,unsigned short data_type,unsigned int max_data_len );
	void ( *process_query )( struct waf_signature_search_object *,char *,int *);
	void ( *release_query )( struct waf_signature_search_object * );

}waf_signature_search_object;

extern int init_db_object( waf_signature_search_object *db_obj,sqlite3 *db );
extern void clear_data_record( struct waf_signature_search_object *db_obj );
extern void process_query( struct waf_signature_search_object *db_obj,char *sql_str,int *err );
extern int add_field ( struct waf_signature_search_object *db_obj,unsigned short data_type,unsigned int max_data_len );
extern void db_close( sqlite3 *db );
void refresh_object( waf_signature_search_object *db_obj );
extern int db_open( sqlite3 **db, char *db_name );

#endif 
