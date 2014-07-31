#ifndef _HTTP_H_
#define _HTTP_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "http_decode.h"
#include "apr_pools.h"
#include "apr_tables.h"

#include "common.h"

#define HTTP_LOGIC_FALSE        0
#define HTTP_LOGIC_TRUE         1

#define HTTP_FORM_BEGIN 0
#define HTTP_FORM_HEADER 1
#define HTTP_FORM_BODY 2
#define HTTP_FORM_ARG 0
#define HTTP_FORM_FILE 1

enum {
        HTTP_STATE_INIT = 0,
        HTTP_STATE_HEAD,
        HTTP_STATE_BODY,
        HTTP_STATE_NONE,
        HTTP_STATE_ERROR,
        HTTP_STATE_NUM,
};
enum {
        HTTP_METHOD_NONE,
        HTTP_METHOD_NA,
        HTTP_METHOD_GET,
        HTTP_METHOD_POST,
        HTTP_METHOD_HEAD,
        HTTP_METHOD_OPTION,
        HTTP_METHOD_PUT,
        HTTP_METHOD_CONNECT,
        HTTP_METHOD_DELETE,
        HTTP_METHOD_TRACE,
        HTTP_METHOD_WEBDAV,
        HTTP_METHOD_NUM,
};


typedef struct _http_local{
        int             sub_status;
        int             data_flag;
        int             clen_isset_0;
        int             clen_isset_1;
        int             unrcv_len_0;
        int             unrcv_len_1;
        int             tenc_0; // transfer incoding
        int             tenc_1; // transfer incoding

	int  		un_recv_content_len;

        int             un_recv_chunked_len; //un recv chunk lenhhhhhh
        int             chunk_flag;
        char            cache_buf[512];
        int             cache_buf_len;

        int             state;
	int 		un_recv_len;
        int             form_status;
        int             form_flag;
        int             subject_flag;
        int             cert_flag;
        int             realip_flag;
        int             abs_url_flag;
        int             header_buf_error;
        int             args_buf_error;
        int             amf_error;
        int             file_num_error;

#define LABEL_STATE_BEGIN 1
#define LABEL_STATE_RUN   2
#define LABEL_STATE_NULL  3

        int             myform_label_num;
        int             myform_label_state;
        int             null_file;
        int             form_header_state;
        int             boundary_cnt;
        int             last_len;
        int             have_8341;
        int             force_decode;
} local_t;

typedef struct _http_logic{
        int             state_0;
        int             state_1;
        int             dir;            /* direct: request/response */
        short           method;         /* method */
	int		content_type;
	int 		is_chunked;
	int 		is_closed;
} logic_t;


typedef struct _http_int{
        u_int16_t       retcode;      
	int 		header_len;
	int 		content_len;
	int 		max_header_line_len;
	int 		body_len;
	int 		url_arg_len;
	int 		url_arg_cnt;
	int 		header_line_num;
	int 		header_cookie_num;
	int 		header_range_num;
} http_int_t;

typedef struct local_str_{
	char* p;
	int len;
	int max_len;
}x_str_t;
typedef struct _http_str{
        x_str_t      raw_url;   
        x_str_t      url;   
        x_str_t      url_and_arg;   
        x_str_t      host; 
        x_str_t      agent; 
        x_str_t      auth; 
        x_str_t      auth_user; 
        x_str_t      auth_pass; 
        x_str_t      reference; 
        x_str_t      location; 
	x_str_t      content_type;
	x_str_t      retcode;
	x_str_t      boundary;
	x_str_t      cookie;
} http_str_t;

typedef struct _http_arg{
        x_str_t      name;
        x_str_t      value;
        int 		complete_flag;
        int             arg_flag;
        int             decode_flag;
} http_arg_t;

typedef struct _http_argctrl{
        http_arg_t      *cur;
        short           num;
        short           new_num;
        short           max_num;
        http_arg_t      arg[32];
}http_arg_ctrl_t;

typedef struct _http_file{
#define HTTP_FILE_MAX_LEN 550
       	x_str_t      name;
        x_str_t      value;
        int complete_flag;
        int raw_file_len;
        int illegal_filesize_logged; //logged already or not?
}http_file_t;

typedef struct _http_fctrl{
        http_file_t*    cur;
        http_file_t    file[8];
        short           num;
        short           new_num;
        short           max_num;
}http_file_ctrl_t;


typedef struct _http_{

	local_t local;
	logic_t logic;
	http_int_t http_int;
	http_str_t str;
	http_arg_ctrl_t arg;
	http_file_ctrl_t file;
	apr_pool_t      *mp;
	int p_cnt;
	int init_flag;
	int tmp_flag;
	void* ssn;
} http_t;

int http_global_init();
int http_init(http_t* info,void* ssn);
int http_clean(http_t* info);

int s_http_parse(char* data,int data_len,http_t* http);
int http_parse_head(char* data,int data_len,int offset,http_t* info,int dir);
int http_parse_body(char* data,int len,http_t* info);



#endif

