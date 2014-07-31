#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sqlite_db.h"

int add_field ( struct waf_signature_search_object *db_obj,unsigned short data_type,unsigned int max_data_len )
{
	
	if( db_obj == NULL )
		goto fail;

	int field_index = db_obj -> field_list_sz;
	

	if( field_index >= MAX_FIELD_NUMBER )
	{	
		printf( "add_field:Field number overflow\n" );
		goto fail;
	}

	if( max_data_len > DATA_LEN_LIMIT )
	{
		printf(" add_field: max data len overflow \n");
		goto fail;
	}	
	
	if( db_obj -> field_list[ field_index ].data_type != 0  )
	{	
		printf("add_field:data type initial error\n");	
		goto fail;
	}	

	db_obj -> field_list[ field_index ].data_type = data_type;
	db_obj -> field_list[ field_index ].max_data_len = max_data_len;

	db_obj -> field_list_sz = db_obj -> field_list_sz + 1;

	return ADD_FIELD_SUCCESS;
		
	fail:	
		db_obj -> fail_record = 1;
		return ADD_FIELD_FAIL;

}	

int db_open( sqlite3 **db ,char *db_name)
{
 	int result = 0;

	result = sqlite3_open(db_name, db  );	

	if (result != SQLITE_OK)
	{
		printf("init_db_object:open database failed\n");
		return -1;

	}	

	return 0;

}	

int init_db_object( waf_signature_search_object *db_obj,sqlite3 *db )
{
	if( db_obj == NULL )
		return -1;
	
	memset( db_obj,0x0,sizeof( waf_signature_search_object ) );

	db_obj -> release_query = clear_data_record;
	db_obj -> process_query = process_query;
	db_obj -> add_field = add_field;
	
	//db_obj -> db_close = db_close;

	/*
	strlcpy( db_obj -> sql_str,sql,MAX_SQL_STR_LEN );
	
	result = sqlite3_open(FWB_SIG_DB_NAME, & (db_obj->db ) );
	
	if (result != SQLITE_OK)
	{
		printf("init_db_object:open database failed\n");
		db_obj -> fail_record = 1;
		return -1;

	}*/

	if( db == NULL )
	{
		printf(" init_db_object fail,db = NULL\n");
		return -1;
	}	

	db_obj -> db = db;	

	return 0;

}


data_record_t  *fill_record( struct waf_signature_search_object *db_obj,sqlite3_stmt *stat,int *err,data_record_t *previous,int data_len )
{
	field_info_t *fi = NULL;
	int field_count = 0 ;
	data_record_t *record = NULL;
	void *data = NULL;
	int offset = 0;
	int i = 0;
	char *text_field = NULL;
	int int_field = 0;
	int len = 0;
	int total_len = 0;
	int text_len = 0;
	
	total_len = data_len;

	fi = db_obj -> field_list;
 	field_count = db_obj -> field_list_sz;

	record = malloc( sizeof( data_record_t ) );

	if( record == NULL )
	{
		printf("Alloc memory for record failed \n");
		*err = ALLOC_RECORD_MEM_FAIL;
		goto fail;
	}
	
	memset( record ,0x0,sizeof( data_record_t ) );
	
	data = malloc( data_len );

	if( !data )
	{
		printf("Alloc memory for data failed\n");
		*err = ALLOC_DATA_MEM_FAIL;
		goto fail;
	}	
	
	memset( data,0x0,data_len );

	record -> data = data;	

	if( db_obj -> record == NULL )
		db_obj -> record = record;	
	else
		previous -> next = record;

	for( i = 0 ; i < field_count ; i ++ )
	{
		if( fi[ i ].data_type == DB_TYPE_INT )
		{	
			len = sizeof( int );
			
			if( total_len < len )
			{
				printf("data left length not enough");
				*err = INT_DATA_LEN_OVERFLOW;
				goto fail;
			}	

			int_field = sqlite3_column_int(stat, i);
			memcpy( data + offset ,&int_field,len );
			offset = offset + len;
			total_len = total_len - len;

		}
		else if( fi[ i ].data_type == DB_TYPE_STRING )
		{
			len = fi[ i ].max_data_len;
			
			if( total_len < len )
			{
				printf("data left length not enough");
				*err = TEXT_DATA_LEN_OVERFLOW;
				goto fail;
			}	
			
			text_field = (char * )sqlite3_column_text(stat, i);

			if( !text_field )
			{
				//printf( "Get text field %d failed\n ",i );
				*err = GET_TEXT_FIELD_FAIL;
				goto add_offset_only;
			}	
			
			text_len = sqlite3_column_bytes(stat, i);
			
			if( text_len > len || text_len < 0 )
			{
				printf("Text length overflow\n");
				printf("Text Field = %s\n",text_field);
				*err = TEXT_LEN_OVERFLOW;
				goto fail;
			}	

			memcpy( data + offset,text_field,text_len );

			add_offset_only:
			offset = offset + len;
                        total_len = total_len - len;

		}	
		else
		{	
			*err = INVALID_DATA_TYPE;
			goto fail;			
		}	
	
	}
	
	
	/*
	signature_rule_db *dd = ( signature_rule_db *)data;
	
	printf("--------------------------------------------\n");
	printf("case senti:%d\n",dd -> case_sensitive );	
	printf("regexp:%s\n",dd -> regexp);

	printf("signature_id :%s\n",dd -> signature_id );
	printf("--------------------------------------------\n");*/
	
	*err = 0;	
	db_obj -> record_count = db_obj -> record_count + 1;
	
	return record;	

	fail:
	
	db_obj -> record_count = 0;
	clear_data_record( db_obj );			
	return NULL;

}	

void clear_data_record( struct waf_signature_search_object *db_obj )
{
	data_record_t *record = NULL,*tmp_record = NULL;

	if( db_obj == NULL )
	{	
		printf("clear_data_record:Invalid db_obj");
		return;
	}

	record = db_obj -> record;

	while( record )
	{
		tmp_record = record -> next;

		if( record -> data )
			free( record -> data );
		
		free( record );

		record = tmp_record;
	}	
	
	db_obj -> record = NULL;
	
	refresh_object( db_obj );

	return;
		
}	

int move_data_to_record( struct waf_signature_search_object *db_obj,sqlite3_stmt *stat )
{
	int data_size = 0;
	int field_count = 0;
	int result = 0;
	int i = 0;
	data_record_t *record = NULL;
	int err = 0 ;
	
	field_info_t *fi = NULL;
	fi = db_obj -> field_list;	
	field_count = db_obj -> field_list_sz;

	for( i = 0; i < field_count ; i++ )
	{
		if( fi[ i ].data_type == DB_TYPE_INT )
		{
			data_size = data_size + sizeof( int );
		}
		
		else if( fi[ i ].data_type  == DB_TYPE_STRING )
		{
			if(  fi[ i ].max_data_len <= 0 || fi[ i ].max_data_len > DATA_LEN_LIMIT  )
			{	
				printf("move_data_to_record:data length invalid\n");
				return INVALID_DATA_LENGTH;
			}
			else
				data_size = data_size +  fi[ i ].max_data_len;	
		
		}
		else
		{
			printf("move_data_to_record:Invalid data type\n");
			return INVALID_DATA_TYPE;
		}	
					

	}
	
	while (1) 
	{
		record = fill_record( db_obj,stat,&err,record,data_size );

		if( !record )
			return err;

		result = sqlite3_step( stat );	

		if (result != SQLITE_ROW)
			break;	

	}	

	return 0;
	

			
	
	
}	


void refresh_object( waf_signature_search_object *db_obj )
{
	if( db_obj )
	{
		db_obj -> field_list_sz = 0;
		db_obj -> record = NULL;
		db_obj -> fail_record = 0;
		db_obj -> record_count = 0;
		memset( db_obj -> field_list ,0x0,MAX_FIELD_NUMBER * sizeof( field_info_t ) );
	}

}	

void process_query( struct waf_signature_search_object *db_obj,char *sql_str,int *err )
{
	int result = 0;
	sqlite3 *db = NULL;
	sqlite3_stmt *stat;

	*err = 0;
	
	if( !db_obj )
	{	
		*err = -1;
		return;
	}

	if( db_obj -> fail_record != 0 )	
	{
		*err = -2;
		return;
	}
	
	db = db_obj -> db;

	if( !db )
	{
		*err = -3;
		return;
	}	
	
	if( strlen( sql_str ) > MAX_SQL_STR_LEN )
	{
		printf("sql string is too long\n");
		*err = -4;
		return;
	}	

	/*
	result = sqlite3_open(FWB_SIG_DB_NAME, &db); 

	if (result != SQLITE_OK) 
	{
		printf("process_query:open database failed\n");
		*err = OPEN_DB_FAIL;
		return;
	}*/
	
	result = sqlite3_prepare(db, sql_str, -1, &stat, 0);
	
	if (result != SQLITE_OK) 
	{
		printf(" process_query:sqlite3_prepare: error.\n");
		*err = PREPARE_DB_ERROR;
		goto fail;
	}

	result = sqlite3_step(stat);
	
	if (result != SQLITE_ROW)
	{
		//printf("process_query:sqlite step error\n");
		//*err = STEP_DB_ERROR;
		goto success;
	}
	
	*err = move_data_to_record( db_obj,stat);

	if( *err )
	{	
		printf("move data failed\n");
		goto fail;		
	}
	
	sqlite3_finalize(stat);
   	
	success:
		return;

	fail:

	sqlite3_finalize(stat);
   	
	return;
}

void db_close( sqlite3 *db )
{
	if( db )
		sqlite3_close( db );
}	

