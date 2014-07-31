
#include "http_parse.h"
#include "http_decode.h"
#include "cfgcommon.h"
#include "http_debug.h"
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#define HTTP_MAX_DECBUF		8192
#define	HTTP_TYPE_NAMELEN	36
#define	HTTP_MAX_TOKENLEN	16
#define HTTP_PARSE_TABSIZE	2
static int http_end_len=8;//strlen("http/1.1");

#define HTTP_FORM_BEGIN 0
#define HTTP_FORM_HEADER 1
#define HTTP_FORM_BODY 2
#define HTTP_FORM_ARG 0
#define HTTP_FORM_FILE 1

int _http_parse_args(http_info_t* info,char* data,int data_len,int in_url);
http_config_t http_config;
int http_set_error(http_info_t* info,int error);
int get_transfer_encoding(http_info_t* info);
int parse_null(char* data,int data_len,int offset,http_info_t* info);
int http_move_header(http_info_t* info);
int http_move_args(http_info_t* info);
extern char * session_get_header(void *s, size_t *len, int dir);
#define NORMAL_ERR -1
#define MEM_ERR -2
#define VER_ERR -3

#define HTTP_CHECK_ERR(a) do{if(unlikely (a == NULL)){ return NORMAL_ERR;}}while(0)
#define HTTP_CHECK_0(a) do{if(unlikely (a == NULL)){ return 0;}}while(0)
#define HTTP_CHECK_NULL(a) do{if(unlikely (a == NULL)){ return NULL;}}while(0)

#define HTTP_NORMAL_ERR(info,a) do{if (unlikely (info == NULL)){return NORMAL_ERR;}if(unlikely (a == NULL)){ info->error_line=__LINE__;return NORMAL_ERR;}}while(0)
#define HTTP_NORMAL_0(info,a) do{if (unlikely (info == NULL)){return 0;} if (unlikely(a == NULL)){ info->error_line=__LINE__;return 0;}}while(0)
#define HTTP_NORMAL_NULL(info,a) do{if (unlikely (info == NULL)){return NULL;} if (unlikely(a == NULL)){ info->error_line=__LINE__;return NULL;}}while(0)

#define HTTP_MEM_0(info,item) do{if (unlikely(item == NULL)){ info->mem_error=1;info->error_line=__LINE__;return 0;}}while(0)
#define HTTP_MEM_NULL(info,item) do{if (unlikely(item == NULL)){ info->mem_error=1;info->error_line=__LINE__;return NULL;}}while(0)
#define HTTP_MEM_ERR(info,item) do{if (unlikely(item == NULL)){ info->mem_error=1;info->error_line=__LINE__;return MEM_ERR;}}while(0)
			
#define CHECK_MEM(ret) do{if (unlikely(ret == MEM_ERR)){ return MEM_ERR;}}while(0)
#define CHECK_VER(ret) do{if (unlikely(ret == VER_ERR)){ return VER_ERR;}}while(0)

#define HTTP_FUNC_0(info) do{if (unlikely(info == NULL)|| unlikely(info->http== NULL)){ printf(" call http use error para %d\n",__LINE__);return 0;}}while(0)
#define HTTP_FUNC_NULL(info) do{if (unlikely(info == NULL)|| unlikely(info->http== NULL)){ printf(" call http use error para %d\n",__LINE__);return NULL;}}while(0)
//#define HTTP_FUNC_0(info) do{if (unlikely(info == NULL)|| unlikely(info->http== NULL)){ return 0;}}while(0)
//#define HTTP_FUNC_NULL(info) do{if (unlikely(info == NULL)|| unlikely(info->http== NULL)){ return NULL;}}while(0)
//#define debug_data 1
#if debug_data
static char mybuf[40960];
int mybuf_len;
char * session_get_origin_header(void *s, size_t *len, int dir)
{
	return &mybuf[0];
}
#else 
#include "session.h"
#endif

enum {
        HTTP_HSTATE_CHAR0,	//before start
        HTTP_HSTATE_CHAR1,	//start
        HTTP_HSTATE_CHARN,	//going on
        HTTP_HSTATE_R1,		///r
        HTTP_HSTATE_N1,		///r/n
        HTTP_HSTATE_R2,		///r/n/r
        HTTP_HSTATE_N2,		///r/n/r/n
        HTTP_HSTATE_NUM,	
};

typedef int (*http_func_t)(char *data, int dlen,int off, http_info_t* i);

typedef struct _http_state
{
        int		state;
        http_func_t	func;
} http_state_t;

typedef struct _http_calltbl
{
	char		key[128];
	int		keylen;
	int		value;
        http_func_t	func;
} http_calltbl_t;


typedef struct _http_hstate
{
        int		state;
} http_hstate_t;

typedef int (*http_parse_func_t)(char *begin, char *end, http_info_t *info);

typedef struct _http_parse_node {
	char		token[HTTP_MAX_TOKENLEN];
	int		token_len;
	http_parse_func_t func;
} http_parse_node_t;

typedef struct _http_ctype {
	char	name[HTTP_TYPE_NAMELEN];
	int	namelen;
	int	id;
} http_ctype_t;

http_state_t		http_mstate[HTTP_STATE_NUM];
#define MAX_HEAD_TABLE_MIX 5
http_calltbl_t		http_header[27][27][27][MAX_HEAD_TABLE_MIX];

#define MAX_METHOD_TABLE_MIX 5
http_calltbl_t		http_methods[27][27][MAX_METHOD_TABLE_MIX];
http_hstate_t		http_hstate[HTTP_HSTATE_NUM][3];
http_ignore_param_t	http_ignore_param;

http_ctype_t http_ctype_table[] = {
	{"text/xml",		8,	HTTP_TYPE_XML},
        {"application/xml",	15,	HTTP_TYPE_XML},
        {"application/rss xml",	19,	HTTP_TYPE_RSS_XML},
        {"application/soap xml",20,	HTTP_TYPE_SOAP},
        {"application/xhtml xml",21,	HTTP_TYPE_XHTML_XML},
        {"multipart/related",	17,	HTTP_TYPE_MR},
        {"multipart/form-data",	19,	HTTP_TYPE_MULTIFORM},
        {"text/html",		9,	HTTP_TYPE_HTML},
        {"application/x-www-form-urlencoded", 33, HTTP_TYPE_FORMENC},
        {"text/plain",		10,	HTTP_TYPE_PLAIN},
        {"application/x-amf",	17,	HTTP_TYPE_AMF},
        {"text/css",		8,	HTTP_TYPE_CSS},
        {"application/x-javascript", 24, HTTP_TYPE_JS},
        {"application/javascript", 22,	HTTP_TYPE_COMMON_JS},
        {"text/javascript",	15,	HTTP_TYPE_TEXT_JS},
        {"multipart/x-mixed-replace", 25, HTTP_TYPE_MIX_XML},
        {"message/http", 12, HTTP_TYPE_MESSAGE},
        {"application/rpc",	15,	HTTP_TYPE_RPC}
};
#define	HTTP_CTYPE_TABLE_SIZE	(sizeof(http_ctype_table)/sizeof(http_ctype_t))




int http_set_str(http_info_t* info,http_str_t* str,char* data,int len);
int get_post_boundary(char *begin, char *end, http_info_t *info);
int http_get_ctype(char *begin, char *end, http_info_t *info);

http_parse_node_t parse_table[HTTP_PARSE_TABSIZE] = {
	{"boundary=", 9, get_post_boundary},
	{"type=", 5, http_get_ctype}
};


int http_split_buf(char **name, int* name_len, char** value, int* value_len, 
		   char* split, int split_len, char* data, int data_len, 
		   int trim_blank,http_info_t* info)
{
        char *p=NULL;
        int i=0;
        if(unlikely(name ==NULL|| name_len==NULL || value ==NULL || value_len == NULL ||split == NULL ||split_len <=0
                || data == NULL || data_len <=0)){
	//	assert(0);
                return NORMAL_ERR;
        }
        p=strlstr(data,data_len,split,split_len);
        if( p == NULL){
                return NORMAL_ERR;
        }
        *name=data;
        *name_len=p-*name;
        *value=p+1;
        *value_len=data+data_len-*value;

        if( trim_blank){
                i=0;
                while( isspace(*name[i] ) && *name<p && (*name_len)>0){
                        (*name)++;
                        (*name_len)--;
                }
                while( isspace((*name)[*name_len-1]) && (*name_len)>0){
                        (*name_len)--;
                }
                i=0;
                while( isspace(*value[i] )&& *value<data+data_len && (*value_len)>0){
                        (*value)++;
                        (*value_len)--;
                }
                while( isspace((*value)[*value_len-1]) && (*value_len)>0){
                        (*value_len)--;
                }

        }
        return 0;

}

int http_get_flag(char* data,int len,int* flag_url,int* flag_arg,int* url_len,http_info_t* info)
{
	assert(data);
	assert(url_len);
	int*  flag=flag_url;
	int i=0;
	*url_len=len;
	for( i=0;i<len;i++){
		switch(data[i]){
			case '%':
                        case '+':
                                SET_NEED_URL_DECODE(*flag);
                                break;
                        case '&':
                                SET_NEED_HTML_DECODE(*flag);
                                break;
                        case '\\':
                                SET_NEED_ESCAPE_DECODE(*flag);
				break;
			case '?':
				*url_len=i;
				flag=flag_arg;
				return 0;

				break;
			default:
				break;
		}
	}
	return 0;
}
char * http_jump_slash(char* str,int len,http_info_t* info)
{
        char *begin=str;
        if( unlikely(begin == NULL || len <=0)){
		assert(0);
                return NULL;
        }
        while(begin<str+len && (*begin == '/' || *begin == '\\')){
                begin ++;
        }
        return begin;
}

char * http_jump_no_slash(char* str,int len,http_info_t* info)
{
        char *begin=str;
        if( unlikely(begin == NULL || len<=0)){
		assert(0);
                return NULL;
        }
        while(begin<str+len &&*begin != '/' && *begin != '\\'){
                begin ++;
        }
        return begin;
}

int http_abs_url_check(http_info_t* info,char* url,int url_len)
{
	assert(url);
        char *begin=url;
        char *end=url+url_len;
        char *host_begin=NULL;
        char *host_end=NULL;
	int ret=0;
	http_protocol_t* pro=http_get_protocol_t(info);
	http_local_t* local=http_get_local_t(info);

	static int http_label_len=7;//strlen("http://");
	static int https_label_len=8;//strlen("https://");
        if( strlstr(url,http_label_len,"http://",http_label_len) == NULL){
                if(  strlstr(url,https_label_len,"https://",https_label_len)== NULL){
                        return url_len;
                }else{

                }
        }else{
        }

        begin=http_jump_no_slash(begin,end-begin,info);
	HTTP_NORMAL_ERR(info,begin);
        begin=http_jump_slash(begin,end-begin,info);
	HTTP_NORMAL_ERR(info,begin);
        host_begin=begin;
        begin=http_jump_no_slash(begin,end-begin,info);
        if( begin == NULL){
                return url_len;
        }
        host_end=begin;
        begin=http_jump_slash(begin,end-begin,info);
        if( begin == NULL){
                return url_len;
        }
        ret=http_set_str(info,&pro->host,host_begin, host_end - host_begin );
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
        	return end-begin+1;
	}
        local->abs_url_flag=1;
        //strlcpy2(url,url_len,begin-1,end-begin+1);
        if( end-begin+1>0){
        	return end-begin+1;
	}else{
        	return url_len;
	}

}
int trim_blank(char* str,int str_len,http_info_t* info)
{
        int i=0;
        int j=0;
        if( unlikely(str == NULL || str_len<=0 )){
                return 0;
        }
        for( i=str_len-1;i>=0;i--){
                if (isblank(str[i])!=0){
                        str[i]='\0';
                        str_len--;
                }else{
                        break;
                }
        }
        for( i=0;i<str_len;i++){
                if (isblank(str[i])!=0){
                }else{
                        break;
                }
        }
        for( i=i;i<str_len;i++){
		str[j]=(str[i]);
		j++;
        }
        str[j]='\0';
        return j;
}

#define A_PRIVATE_NETPART_WORTH (10)  /* 10.0.0.0/8 netpart bits:00001010*/
#define B_PRIVATE_NETPART_WORTH (2753) /* 172.16.0.0/12 netpart bits: 10101100 0001*/
#define C_PRIVATE_NETPART_WORTH (49320) /* 192.168.0.0/16 netpart bits: 11000000 10101000 */

int http_check_private_ip_addr(unsigned int net_order_ip)
{
        int ret = 0;
        unsigned int host_order_ip = 0;


        host_order_ip = ntohl(net_order_ip);

        if (((host_order_ip >> 24) & 0xFF) == A_PRIVATE_NETPART_WORTH) {
                ret = 1;
                goto out;
        }

        if (((host_order_ip >> 20) & 0x0FFF) == B_PRIVATE_NETPART_WORTH) {
                ret = 1;
                goto out;
        }

        if (((host_order_ip >> 16) & 0xFFFF) == C_PRIVATE_NETPART_WORTH) {
                ret = 1;
                goto out;
        }

out:
        return ret;
}


int http_get_left_ip(char* data,int len,http_info_t* info,char* buf,int buf_len)
{
	assert(data);
	assert(buf);
        char* begin=data;
        char* end=NULL;
        struct in_addr addr;
        while( begin<data+len){
                end=strlstr(begin,data+len-begin,",",1);
                if( end == NULL){
                        end=data+len;
                }
                if (strlstrip(&begin, &end) <= 0) {
                        begin=end+1;
                        continue;
                }
                strlcpy2(buf,buf_len,begin,end-begin);
                if (inet_aton(buf, &addr) == 0) {
                        begin=end+1;
                        continue;
                }
                if( http_check_private_ip_addr((unsigned int) (addr.s_addr)) == 1){
                        begin=end+1;
                        continue;
                }
	                return 0;
        }

        return NORMAL_ERR;

}



int get_ori_ip(char *begin, char *end, http_info_t* info)
{
	assert(begin);
	assert(end);
	http_protocol_t* pro=http_get_protocol_t(info);
	assert(pro);
	char buf[128];
	int ret=0;

	begin=strlstr(begin,end-begin,":",1);

	if( begin == NULL ||begin >=end ){
                goto out;
	}
	begin=begin+1;
       if (strlstrip(&begin, &end) <= 0) {
                goto out;
        }
        char *q=NULL;
	if( http_config.ip_location == 0){
                if( http_get_left_ip(begin,end-begin,info,buf,sizeof(buf)) == 0){
			ret=http_set_str(info,&pro->origin_ip,buf,strlen(buf));
			CHECK_MEM(ret);
			if( ret == NORMAL_ERR){
				return NORMAL_ERR;
			}
                }
        }else{


	        q=strrlstr(begin,end-begin,",",1);
		if( q == NULL){
			q=begin; 
		}else{ 
			q=q+1;
		} 
		ret=http_set_str(info,&pro->origin_ip,q, end -q);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
	pro->origin_ip.len=trim_blank(pro->origin_ip.p,pro->origin_ip.len,info);

	if( pro->origin_ip.len == 0){
		return 0;
	}
        struct in_addr addr;
        if (inet_aton(pro->origin_ip.p, &addr) == 0) {
		pro->origin_ip.p="\0";
		pro->origin_ip.len=0;
        }


out:
        return 0;
}




void * 
http_malloc( http_info_t* info,int size)
{
	char *p=apr_palloc(info->mp, size);
        return  p;
}

http_arg_t * 
http_array_new_arg(http_info_t* info)
{
	http_argctrl_t* arg=http_get_arg_t(info);
	http_arg_t* item=http_malloc(info,sizeof(http_arg_t));
	HTTP_MEM_NULL(info,item);
	memset(item,0x00,sizeof(http_arg_t));
	arg->new_num++;
	*(http_arg_t**)apr_array_push(arg->array)=item;
	return item;
} 

http_cookie_t * 
http_array_new_cookie(http_info_t* info)
{
	http_ckctrl_t* cookie=http_get_cookie_t(info);
	http_cookie_t* item=http_malloc(info,sizeof(http_cookie_t));
	HTTP_MEM_NULL(info,item);
	memset(item,0x00,sizeof(http_cookie_t));
	*(http_cookie_t**)apr_array_push(cookie->array)=item;
	return item;
}

int  
http_append_data_buf(http_combuf_t* buf,char** data,int *data_len, 
		int max_len,char* value,int value_len)
{
        int offset=0;
        if( buf == NULL || data == NULL || data_len == NULL|| value == NULL ){
		//HTTP_NORMAL_NULL(info,NULL);
                return 0;
        }
        if( value_len > max_len){
                goto err;
        }
        if( buf->current+value_len>= buf->max_len){
                goto err;
        }
        offset=buf->current;
	assert(buf->buf);
        memlcpy(&buf->buf[buf->current],buf->max_len-buf->current,value,value_len);

	if( *data == NULL){
        	*data=&buf->buf[buf->current];
        	*data_len=value_len;
	}else{
		if( value[0] == '\0' && value_len == 1){
		}else{
	        	*data_len=*data_len+value_len;
		}
	}
       	buf->current=buf->current+value_len;

        return 0;
err:
	if( *data == NULL){
        	*data="\0";
        	*data_len=0;
	}
        return NORMAL_ERR;
}



http_argctrl_t * 
http_get_arg_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	return &info->http->arg;
}

http_amfctrl_t * 
http_get_amf_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	return &info->http->amf;
}

http_int_t * 
http_get_int_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	return &info->http->hint;
}

http_local_t * 
http_get_local_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	return &info->http->local;
}

http_logic_t * 
http_get_logic_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	return &info->http->logic;
}

http_config_t * 
http_get_config_t(http_info_t* info)
{
	assert(info);
	return &info->cfg;
}

int http_set_clen(http_info_t* info,int len)
{
	http_logic_t* logic=http_get_logic_t(info);
        http_int_t* http_int=http_get_int_t(info);
        if ( logic->dir == 0){
                http_int->clen_0=len;
        }else{
                http_int->clen_1=len;
        }
	logic->is_closed = HTTP_LOGIC_FALSE;


	return 0;
}

int 
http_set_ctype(http_info_t* info,int type)
{
	http_logic_t* logic=http_get_logic_t(info);
        if ( logic->dir == 0){
                logic->ctype_0=type;
        }else{
                logic->ctype_1=type;
        }

	return 0;
}

int http_set_status(http_info_t* info,int status)
{
	if( unlikely(info)){
	}
	if( unlikely(info->http == NULL)){
		printf(" call http_set_status error\n");
		return 0;
	}
	http_logic_t* logic=http_get_logic_t(info);
	assert(logic);
        if ( logic->dir == 0){
                logic->state_0=status;
        }else{
                logic->state_1=status;
        }

	return 0;
}





char * 
http_get_buf(struct _http_info *info,int index)
{
	HTTP_FUNC_NULL(info);

	http_logic_t* logic=http_get_logic_t(info);
	assert(logic);
	switch( index){
		case HTTP_HEADBUF:
			if( logic->dir == 0){
				return  info->http->header_0.buf.buf;
			}else{
				return  info->http->header_1.buf.buf;
			}
			break;
		case HTTP_ARGBUF:
			return  info->http->arg.buf.buf;//info->multi_buf->common[COMMON_BUF_URL_AND_ARGS].buf;
			break;
		case HTTP_AMFBUF:
			return  info->http->amf.buf.buf;//info->multi_buf->common[COMMON_BUF_URL_AND_ARGS].buf;
			break;
		default:
			return "\0";
			break;
	}
	return "\0";
}

int 
http_get_buflen(struct _http_info *info,int index)
{
	HTTP_FUNC_0(info);

	http_logic_t* logic=http_get_logic_t(info);
	assert(logic);
	switch( index){
		case HTTP_HEADBUF:
			if( logic->dir == 0){
				return  info->http->header_0.buf.current;
			}else{
				return  info->http->header_1.buf.current;
			}
			break;
		case HTTP_ARGBUF:
			return  info->http->arg.buf.current;
			break;
		case HTTP_AMFBUF:
			return  info->http->amf.buf.current;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}

int 
http_get_hline_cnt(http_info_t* info)
{
	HTTP_FUNC_0(info);

	http_hctrl_t *ctl=http_get_header_t(info);
	assert(ctl);
	assert(ctl->array);
	return ctl->array->nelts;
}

http_hline_t * 
http_get_hline(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);
        http_hctrl_t* header=http_get_header_t(info); 
	assert(header);
	assert(header->array);
	if( num >=header->array->nelts){
		return NULL;
	}
        return  ((http_hline_t **)header->array->elts)[num];
}

int 
http_get_header_line_cnt(http_info_t* info)
{

	HTTP_FUNC_0(info);
	http_hctrl_t *ctl=http_get_header_t(info);
	assert(ctl);
	assert(ctl->array);
	return ctl->array->nelts-1;
}

http_header_line_t * 
http_get_header_line(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);

        http_hctrl_t* header=http_get_header_t(info);
	assert(header);
	assert(header->array);
	if( num+1 >=header->array->nelts){
		return NULL;
	}

        http_hline_t* line=((http_hline_t **)header->array->elts)[num+1];
	HTTP_CHECK_NULL(line);
        return  (http_header_line_t*) &line->name;
}


int 
http_get_file_cnt(http_info_t* info)
{
	HTTP_FUNC_0(info);
	http_fctrl_t *file=http_get_file_t(info);
	assert(file);
	assert(file->array);
	int num=file->array->nelts;
	if( file->cur == NULL){ 
		return num;
	}
	if( file->cur->complete_flag==1){
                return num;
        }else{
                if( num-1>0){
                        return num-1;
                }else{
                        return 0;
                }
        }

}

int 
http_get_av_cnt(http_info_t* info)
{
	HTTP_FUNC_0(info);
	http_avctrl_t *av=http_get_av_t(info);
	assert(av);
	assert(av->array);
	return av->array->nelts;
}

http_file_t * 
http_get_file(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);

        http_fctrl_t* file=http_get_file_t(info);
	assert(file);
	assert(file->array);
	if( num >=file->array->nelts){
		return NULL;
	}

        return  ((http_file_t **)file->array->elts)[num];
}

http_av_t * 
http_get_av(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);
        http_avctrl_t* av=http_get_av_t(info);
	assert(av);
	assert(av->array);
	if( num >=av->array->nelts){
		return NULL;
	}

        return  ((http_av_t **)av->array->elts)[num];
}

int 
http_get_arg_cnt(http_info_t* info)
{
	HTTP_FUNC_0(info);
	http_argctrl_t *arg=http_get_arg_t(info);
	assert(arg);
	assert(arg->array);
	int num=arg->array->nelts;
	if( arg->cur == NULL){
		return num;
	}
        if( arg->cur->complete_flag==1){
                return num;
        }else{
                if( num-1>0){
                        return num-1;
                }else{
                        return 0;
                }
        }

}

http_arg_t * 
http_get_arg(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);
        http_argctrl_t* ctl=http_get_arg_t(info);
	assert(ctl);
	assert(ctl->array);
	if( num >= ctl->array->nelts){
		return NULL;
	}

        http_arg_t* arg=((http_arg_t **)ctl->array->elts)[num];
	if( arg->name.p== NULL){
		arg->name.len=0;
		arg->name.p="\0";
	}else if( arg->name.len ==1 && arg->name.p[0]=='\0'){
		arg->name.len=0;
	}
	if( arg->value.p== NULL){
		arg->value.len=0;
		arg->value.p="\0";
	}else if( arg->value.len ==1 && arg->value.p[0]=='\0'){
		arg->value.len=0;
	}
        return  arg;
}

int 
http_get_cookie_cnt(http_info_t* info)
{
	HTTP_FUNC_0(info);
	http_ckctrl_t *cookie=http_get_cookie_t(info);
	assert(cookie);
	assert(cookie->array);
	return cookie->array->nelts;
}
	
http_cookie_t * 
http_get_cookie(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);
        http_ckctrl_t* cookie=http_get_cookie_t(info);
	assert(cookie);
	assert(cookie->array);
	if( num >= cookie->array->nelts){
		return NULL;
	}

        return  ((http_cookie_t **)cookie->array->elts)[num];
}

int 
http_get_amf_cnt(http_info_t* info)
{
	HTTP_FUNC_0(info);
	http_amfctrl_t *amf=http_get_amf_t(info);
	assert(amf);
	assert(amf->array);
	return amf->array->nelts;
}
	
http_amf_t * 
http_get_amf(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);
        http_amfctrl_t* amf=http_get_amf_t(info);
	assert(amf);
	assert(amf->array);
	if( num >= amf->array->nelts){
                printf(" pls dont give me the NULL to get amf \n");
		return NULL;
	}

        return  ((http_amf_t **)amf->array->elts)[num];
}

http_hline_t* http_get_cur_hline(http_info_t* info)
{
	assert(info);
	assert(info->http);
	if( info->http->logic.dir == 0){
		return info->http->header_0.cur;
	}else{
		return info->http->header_1.cur;
	}

}

http_hline_t * 
http_new_hline(http_info_t* info)
{
	
	http_hctrl_t* header=http_get_header_t(info);
	http_hline_t* item=NULL;
	item=http_malloc(info,sizeof(http_hline_t));
	HTTP_MEM_NULL(info,item);
	memset(item,0x00,sizeof(http_hline_t));

	*(http_hline_t**)apr_array_push(header->array)=item;
	return item;
} 

int http_record_line(http_info_t* info, int begin, int len, int split)
{
	http_hline_t* hline = NULL;
	hline = http_get_cur_hline(info);;
	if(hline == NULL){
		return NORMAL_ERR;
	}
	if( split<begin){
		split=begin;
	}
        hline->begin=begin;
        hline->len=len;
        hline->split=split;
        return 0;
}

int http_set_len_flag(http_info_t* info,int flag)
{
	http_logic_t* logic=http_get_logic_t(info);
        http_local_t* local=http_get_local_t(info);
        if ( logic->dir == 0){
                local->clen_isset_0=flag;
        }else{
                local->clen_isset_1=flag;
        }

	return 0;
}

int http_add_un_recv_len(http_info_t* info,int len)
{
	http_logic_t* logic=http_get_logic_t(info);
        http_local_t* local=http_get_local_t(info);
	//len can be less than 0
	if( local->unrcv_len_0<5000){
	//	printf("%d %d\n",local->unrcv_len_0,len);
	}
	if (logic->is_closed == HTTP_LOGIC_TRUE){
		return 0;
	}
        if ( logic->dir == 0){
                local->unrcv_len_0=local->unrcv_len_0+len;
        }else{
                local->unrcv_len_1=local->unrcv_len_1+len;
        }

	if( local->unrcv_len_1<0 || local->unrcv_len_0<0){
		printf(" error\n");
		HTTP_NORMAL_ERR(info,NULL);
	}

	return 0;
}




int  data_set_buf(int flag,http_combuf_t* buf,char** data,int *data_len,int max_len,char* value,int value_len,http_info_t* info)
{
        int offset=0;
	char* mydata=value;
	int mylen=value_len;
	//http_hline_t* header=http_get_cur_hline(info);
        if( unlikely(buf == NULL || data == NULL || data_len == NULL|| value == NULL) ){
		HTTP_NORMAL_ERR(info,NULL);
                return 0;
        }
        if( value_len > max_len){
                goto err;
        }
        if( buf->current+value_len>= buf->max_len){
                goto err;
        }
        offset=buf->current;

	unsigned char sbuf[HTTP_MAX_DECBUF];
	int ret=0;
	if( GET_NEED_DECODE(flag) != 0){
		ret=http_decode_string(sbuf,sizeof(sbuf),(unsigned char*)value,value_len,flag,info->cfg.decode_flag);
		if( ret>0 && ret<= value_len){
			mydata=(char*)&sbuf[0];
			mylen=ret;
		}
	}
	assert(buf->buf);
        memlcpy(&buf->buf[buf->current],buf->max_len-buf->current,mydata,mylen);
	
        *data=&buf->buf[buf->current];
        *data_len=mylen;

       	buf->current=buf->current+mylen;
        buf->buf[buf->current]='\0';
       	buf->current++;
        return 0;
err:
        *data="\0";
        *data_len=0;
        return NORMAL_ERR;
}




int 
http_init_ignore_param(void)
{
        http_bmpat_t *tmp=NULL;

        tmp=&http_ignore_param.params[0];
        tmp->name=HTTP_SM_CKNAME;
        tmp->len=strlen(tmp->name);
        bm_get_tab((unsigned char*)tmp->name,tmp->len,&tmp->jump_tab[0],256);

        tmp=&http_ignore_param.params[1];
        tmp->name=HTTP_PARAM_NAME;
        tmp->len=strlen(tmp->name);
        bm_get_tab((unsigned char*)tmp->name,tmp->len,&tmp->jump_tab[0],256);

        tmp=&http_ignore_param.params[2];
        tmp->name=HTTP_REASON_PARAM;
        tmp->len=strlen(tmp->name);
        bm_get_tab((unsigned char*)tmp->name,tmp->len,&tmp->jump_tab[0],256);

        tmp=&http_ignore_param.params[3];
        tmp->name= HTTP_REDIECT_PARAM;
        tmp->len=strlen(tmp->name);
        bm_get_tab((unsigned char*)tmp->name,tmp->len, &tmp->jump_tab[0], 256);

        http_ignore_param.params[4].name="";
        http_ignore_param.params[4].len=0;

        http_ignore_param.init_flag=1;
        return 0;
}

int 
http_init_hstate(void) 
{
	http_hstate[HTTP_HSTATE_CHAR0][0].state=HTTP_HSTATE_CHAR1;
	http_hstate[HTTP_HSTATE_CHAR0][1].state=HTTP_HSTATE_CHAR0;
	http_hstate[HTTP_HSTATE_CHAR0][2].state=HTTP_HSTATE_CHAR0;

	http_hstate[HTTP_HSTATE_CHAR1][0].state=HTTP_HSTATE_CHARN;
	http_hstate[HTTP_HSTATE_CHAR1][1].state=HTTP_HSTATE_R1;
	http_hstate[HTTP_HSTATE_CHAR1][2].state=HTTP_HSTATE_CHARN;

	http_hstate[HTTP_HSTATE_CHARN][0].state=HTTP_HSTATE_CHARN;
	http_hstate[HTTP_HSTATE_CHARN][1].state=HTTP_HSTATE_R1;
	http_hstate[HTTP_HSTATE_CHARN][2].state=HTTP_HSTATE_CHARN;

	http_hstate[HTTP_HSTATE_R1][0].state=HTTP_HSTATE_CHAR1;
	http_hstate[HTTP_HSTATE_R1][1].state=HTTP_HSTATE_R1;
	http_hstate[HTTP_HSTATE_R1][2].state=HTTP_HSTATE_N1;

	http_hstate[HTTP_HSTATE_N1][0].state=HTTP_HSTATE_CHAR1;
	http_hstate[HTTP_HSTATE_N1][1].state=HTTP_HSTATE_R2;
	http_hstate[HTTP_HSTATE_N1][2].state=HTTP_HSTATE_N1;

	http_hstate[HTTP_HSTATE_R2][0].state=HTTP_HSTATE_R2;
	http_hstate[HTTP_HSTATE_R2][1].state=HTTP_HSTATE_R2;
	http_hstate[HTTP_HSTATE_R2][2].state=HTTP_HSTATE_N2;

	http_hstate[HTTP_HSTATE_N2][0].state=HTTP_HSTATE_CHAR1;
	http_hstate[HTTP_HSTATE_N2][1].state=HTTP_HSTATE_N2;
	http_hstate[HTTP_HSTATE_N2][2].state=HTTP_HSTATE_N2;
	return 0;
}

int http_core_set_header(http_info_t* info,char* name,int name_len,char* value,int value_len)
{
	http_hctrl_t* header=http_get_header_t(info);
	http_hline_t* item=http_new_hline( info);
	if( unlikely(item == NULL)){
		return MEM_ERR;
	}
	http_str_t* hname=&item->name;
	http_str_t* hvalue=&item->value;
	int ret=0;
	ret=data_set_buf(0,&header->buf,&hname->p,&hname->len,name_len,name,name_len,info);
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
		return NORMAL_ERR;
	}
	ret=data_set_buf(0,&header->buf,&hvalue->p,&hvalue->len,value_len,value,value_len,info);
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
		return NORMAL_ERR;
	}
	return 0;
	
}

int 
http_get_ctype(char *begin, char *end, http_info_t *info)
{
	int i;
	int flag=0;

	if( unlikely(begin == NULL || end==NULL || begin>=end || info == NULL)){
		HTTP_NORMAL_ERR(info,NULL);
		return 0;
	}

	for (i = 0; i < HTTP_CTYPE_TABLE_SIZE; i++) {
		if( end-begin < http_ctype_table[i].namelen){
			continue;
		}
		if (!strncasecmp(begin, http_ctype_table[i].name, 
				http_ctype_table[i].namelen)) 
		{	
			 if(begin[http_ctype_table[i].namelen] != ';'
                            && begin[http_ctype_table[i].namelen] != '\r'
                            && begin[http_ctype_table[i].namelen] != '\0'
                            && begin[http_ctype_table[i].namelen] != ' ')
			 {
				 continue;
			 }
			 http_set_ctype(info, http_ctype_table[i].id);
			 flag = 1;

			 /* we should break, added by joseph, on 2007-1-11 */
			 break;
		}
	}
	if( flag == 0){
		http_set_ctype(info,HTTP_TYPE_NA);
	}
	return 0;
}




void parse_table_parsing(char *begin, char *end, http_info_t *di)
{
	int i = 0;
	if( unlikely(begin == NULL || end==NULL || begin>=end || di == NULL)){
		return ;
	}

	for (i = 0; i < HTTP_PARSE_TABSIZE; i++) {
		if ( end-begin< parse_table[i].token_len){
			continue;
		}
		if (!strncasecmp(begin, parse_table[i].token, parse_table[i].token_len)) {
			
			parse_table[i].func(begin + parse_table[i].token_len, 
					end, di);
			break;
		}
	}
}


int http_parse_content_type(char *begin, char *end, http_info_t *info)
{
	char *ptr = NULL;
	if( unlikely(begin == NULL || end==NULL || begin>=end || info == NULL)){
		HTTP_NORMAL_ERR(info,NULL);
		return 0;
	}
	
	ptr = strlstr(begin, end - begin, ";",1);
	if (!ptr) {
		ptr = end;
	}

	http_get_ctype(begin, ptr, info);
	if (http_get_logic(info, HTTP_LOGIC_CTYPE) == HTTP_TYPE_NA) {
		goto out;
	}

	begin = ptr + 1;
	while (begin < end && end-begin<4096) {
		
		ptr = strlstr(begin, end - begin, ";",1);
		if (!ptr) {
			ptr = end;
		}

		/* skip space */
		while ((*begin == ' ' || *begin == '\t') && (begin < ptr)) {
			begin++;
		}
		
		parse_table_parsing(begin, ptr, info);

		begin = ptr + 1;
	}	
out:
	return 0;
}

int http_add_cookie_num(http_info_t* info)
{
	http_logic_t* logic=http_get_logic_t(info);
        http_int_t* http_int=http_get_int_t(info);
        if ( logic->dir == 0){
                http_int->ncookie_0++;
        }else{
                http_int->ncookie_1++;
        }

	return 0;
}




int http_set_cookie(http_cookie_t* item ,http_info_t* info)
{
	assert(item);
	http_cookie_t* cur=http_array_new_cookie(info);
	http_local_t* local=http_get_local_t(info);
	if( unlikely(cur == NULL)){
		return MEM_ERR;
	}


	http_add_cookie_num(info);
	int ret=0;
	ret=http_set_str(info,&cur->name,item->name.p,item->name.len);
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
		return NORMAL_ERR;
	}
	if( strlstr(item->name.p,item->name.len,WAF_PREFIX_NEW,strlen(WAF_PREFIX_NEW))!=NULL){
                local->data_flag=0;
        }

	ret=http_set_str(info,&cur->value,item->value.p,item->value.len);
	CHECK_MEM(ret);
	if( item->path.p != NULL){
		ret=http_set_str(info,&cur->path,item->path.p,item->path.len);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}else{
		ret=http_set_str(info,&cur->path,"\0",1);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
	if( item->domain.p != NULL){
		ret=http_set_str(info,&cur->domain,item->domain.p,item->domain.len);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}else{
		ret=http_set_str(info,&cur->domain,"\0",1);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
	if( item->expire.p != NULL){
		ret=http_set_str(info,&cur->expire,item->expire.p,item->expire.len);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}else{
		ret=http_set_str(info,&cur->expire,"\0",1);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
	return 0;
}





int http_parse_one_cookie(char* data,int len,http_info_t* info)
{
	assert(data);
	char *begin=NULL;
	http_cookie_t ck;
	int ret;
	memset(&ck, 0x00, sizeof(http_cookie_t));

	http_logic_t* logic = http_get_logic_t(info);
	http_protocol_t* pro = http_get_protocol_t(info);
        begin=data;

	if(http_split_buf(&ck.name.p, &ck.name.len, &ck.value.p, 
			  &ck.value.len, "=", 1, begin, len, 1,info)<0)
	{
		 goto out;
	}

	if ( logic->is_wafsid == HTTP_LOGIC_FALSE){
                if (strncasecmp(ck.name.p, HTTP_SM_CKNAME, 
				strlen(HTTP_SM_CKNAME)) ==0 )
		{
			ret=http_set_str(info, &pro->wafsid, ck.value.p, 
				     ck.value.len);
			CHECK_MEM(ret);
			if( ret == NORMAL_ERR){
				return NORMAL_ERR;
			}
			
			logic->is_wafsid = HTTP_LOGIC_TRUE;
		}
        }
	
	ret=http_set_cookie(&ck,info);
	CHECK_MEM(ret);
	if( ret <0){
		goto out;
	}
	


        return 0;

out:

        return NORMAL_ERR;

}




int http_parse_cookie(char* data,int len,http_info_t* info)
{
	
	assert(data);
	char* begin=data;
	char* end=NULL;
	int count=0;
	int ret;
	do{
                end=strlstr(begin,data+len-begin,";",1);
                if ( end== NULL){
                        end=data+len;
                }
		ret=http_parse_one_cookie(begin,end-begin,info);
		CHECK_MEM(ret);
		begin=end+1;
		
	}while(begin!= NULL && end!= NULL && begin<data+len && count++<128);

	
	
	return 0;		
}


int http_parse_set_cookie(char* data,int len,http_info_t* info)
{
	char *name=NULL;
        char *value=NULL;
	int name_len=0;
	int value_len=0;

	assert(data);
	http_cookie_t cookie;
	memset(&cookie,0x00,sizeof(http_cookie_t));

        char * begin=NULL;
        char * end=NULL;

        begin=data;
        int count=0;
        int flag=0;

        do{
                end=strlstr(begin,data+len-begin,";",1);
                if ( end ==  NULL){
                        end=data+len;
                }
                name=NULL;
                name_len=0;
                value=NULL;
                value_len=0;
                if(http_split_buf(&name, &name_len, &value, &value_len, 
				   "=", 1, begin, end-begin, 1,info)<0)
		{
                        begin=end+1;
                        continue;
                }

                if ( name_len==6&&strncasecmp(name,"domain",6) ==0){
                        cookie.domain.p=value;
                        cookie.domain.len=value_len;
                }else  if ( name_len ==7 && strncasecmp(name,"expires",7) ==0){
                        cookie.expire.p=value;
                        cookie.expire.len=value_len;
                }else  if ( name_len==4 &&strncasecmp(name,"path",4) ==0){
                        cookie.path.p=value;
                        cookie.path.len=value_len;
                }else{
                        if( flag == 0){
                                flag=1;
                                cookie.name.p=name;
                                cookie.name.len=name_len;
                                cookie.value.p=value;
                                cookie.value.len=value_len;
                        }
                }
                begin=end+1;

        }while(begin!= NULL && end!= NULL && begin<len+data && count++<128);
	
	int ret=http_set_cookie(&cookie,info);
	CHECK_MEM(ret);	

	return 0;
}






int set_transfer_encoding(http_info_t* info,int status)
{
        http_logic_t* logic=http_get_logic_t(info);
        http_local_t* local=http_get_local_t(info);
	if( logic->dir == 0){
		local->tenc_0=status;
	}else{
		local->tenc_1=status;
	}
	
	return 0;
}
int get_transfer_encoding(http_info_t* info)
{
        http_logic_t* logic=http_get_logic_t(info);
        http_local_t* local=http_get_local_t(info);
	if( logic->dir == 0){
		return local->tenc_0;
	}else{
		return local->tenc_1;
	}
	
}

int http_init_int(http_info_t* info)
{
	http_int_t* http_int=http_get_int_t(info);
	memset(http_int,0x00,sizeof(http_int_t));
	return 0;
}
int http_init_str_after_request(http_info_t* info)
{
	return 0;
}
int http_init_str(http_info_t* info)
{
	http_protocol_t* pro=http_get_protocol_t(info);
	memset(pro,0x00,sizeof(http_protocol_t));
	return 0;
}

int http_append_str(http_info_t* info,http_str_t* str,char* data,int len)
{
	if( unlikely(len<=0)){
		return NORMAL_ERR;
	}
	if( len >HTTP_FILE_MAX_LEN){
		return 0;
	}
	if( str->p == NULL){
		str->p=http_malloc(info,HTTP_FILE_MAX_LEN+1);
		HTTP_MEM_ERR(info,str->p);
	}
	if( str->len+len >HTTP_FILE_MAX_LEN){
                return 0;
        }

	memcpy(str->p+str->len,data,len);	
	str->len=str->len+len;
	str->p[str->len]='\0';
	return 0;
}

int 
http_decode_arg(http_arg_t* item,http_info_t* info)
{
	assert(item);
	unsigned char buf[HTTP_MAX_DECBUF];
	int ret=0;
	if( GET_NEED_DECODE(item->name_flag) != 0){
		ret=http_decode_string(buf,sizeof(buf),(unsigned char*)item->name.p,item->name.len,item->name_flag,info->cfg.decode_flag);
		if( ret>0 && ret<item->name.len){
			memlcpy(item->name.p,item->name.len,buf,ret);
			item->name.len=ret;
			item->name.p[item->name.len]='\0';
		}
	}
	if( GET_NEED_DECODE(item->value_flag) != 0){
		ret=http_decode_string(buf,sizeof(buf),(unsigned char*)item->value.p,item->value.len,item->value_flag,info->cfg.decode_flag);
		if( ret>0 && ret< item->value.len){
			memlcpy(item->value.p,item->value.len,buf,ret);
			item->value.len=ret;
			item->value.p[item->value.len]='\0';
		}
	}
	return 0;
}


int 
http_append_arg(char* value,int value_len,http_info_t* info,http_arg_t* item)
{
	assert(item);
	if( unlikely(value == NULL|| value_len <0)){
		return 0;
	}
	http_argctrl_t *arg=http_get_arg_t(info);
	http_local_t *local=http_get_local_t(info);
	http_logic_t *logic=http_get_logic_t(info);

	http_int_t *http_int=http_get_int_t(info);

	if( value[0] != '\0'){
		http_int->allarg_len=http_int->allarg_len+value_len;
	}
	
	int ret=0;
	http_str_t* aname=&item->name;
	http_str_t* avalue=&item->value;
	if( item->arg_flag ==0){
		ret=http_append_data_buf(&arg->buf,&aname->p,&aname->len,value_len,value,value_len);
		if( ret==NORMAL_ERR){
                	ret=http_move_args(info);
			CHECK_MEM(ret);
			if( ret == NORMAL_ERR){
				return NORMAL_ERR;
			}
			aname->p=NULL;
			aname->len=0;
			ret=http_append_data_buf(&arg->buf,&aname->p,&aname->len,value_len,value,value_len);
			CHECK_MEM(ret);
			if( ret == NORMAL_ERR){
				http_set_error(info,HTTP_ARGBUF_OVERSIZE);
				return NORMAL_ERR;
			}
		}

	}else{
		ret=http_append_data_buf(&arg->buf,&avalue->p,&avalue->len,value_len,value,value_len);
		if( ret==NORMAL_ERR){
                	ret=http_move_args(info);
			CHECK_MEM(ret);
			if( ret == NORMAL_ERR){
				return NORMAL_ERR;
			}
			avalue->p=NULL;
			avalue->len=0;
			ret=http_append_data_buf(&arg->buf,&avalue->p,&avalue->len,value_len,value,value_len);
			if( ret == NORMAL_ERR){
				http_set_error(info,HTTP_ARGBUF_OVERSIZE);
				return NORMAL_ERR;
			}
		}
	}
	if( local->force_decode == 1 || (value[0] == '\0' && item->arg_flag ==1)|| http_get_logic(info,HTTP_LOGIC_STATE)==HTTP_STATE_NONE){
		if( arg->cur!= NULL){
			if( strcmp(arg->cur->name.p,HTTP_REDIECT_PARAM)== 0){
                                        logic->is_wafrdt=HTTP_LOGIC_TRUE;
			}
		}

		http_decode_arg( arg->cur,info);		
		http_int_t *http_int=http_get_int_t(info);
		http_int->nallarg ++;
	}
	return 0;
}




int http_check_hstate(char c,int status,int offset,http_info_t* info)
{
	http_hline_t* header=NULL;
	header=http_get_cur_hline(info);;
	HTTP_CHECK_ERR(header);
	http_local_t *local=http_get_local_t(info);
	switch (c){
		case '\r':
                	return http_hstate[status][1].state;
			break;
		case '\n':
                	return http_hstate[status][2].state;
			break;
		case '%':
			SET_NEED_URL_DECODE(header->flag);	
                	return http_hstate[status][0].state;
			break;
		case '&':
			SET_NEED_HTML_DECODE(header->flag);	
                	return http_hstate[status][0].state;
			break;
		case '\\':
			SET_NEED_ESCAPE_DECODE(header->flag);	
                	return http_hstate[status][0].state;
			break;
		case '+':
			SET_NEED_URL_DECODE(header->flag);	
                	return http_hstate[status][0].state;
			break;
		case ':':
			if( header->split == 0){
				header->split=offset+local->last_len;
			}
		default:
                	return http_hstate[status][0].state;
			break;
			
	}
        return http_hstate[status][0].state;
}

int http_match_hltb(char *begin, char *data, int len, int offset, http_info_t *info)
{
	assert(begin);
	assert(data);
	http_hline_t* hline=NULL;
	hline = http_get_cur_hline(info);;
	HTTP_CHECK_ERR(hline);
	if( len <3){
		parse_null(data,len,offset,info);
		return 0;
	}
	http_calltbl_t *table= http_header[http_char2num(begin[0])][http_char2num(begin[1])][http_char2num(begin[2])];
	int i=0;
	
	int ret=0;
        http_local_t* local=http_get_local_t(info);
        local->data_flag=hline->flag;
	for(i=0;i<MAX_HEAD_TABLE_MIX;i++){
		if(table[i].keylen==0){	
			parse_null(data,len,offset,info);
			break;
		}
		if( table[i].keylen != hline->name.len){
			continue;
		}
		if( http_clean_str_cmp(info,hline->name.p,hline->name.len,table[i].key,table[i].keylen) != 0){
			continue;
		}
		ret=table[i].func(data,len,offset,info);
		CHECK_MEM(ret);
		break;
		
	}
	return 0;
	
}

int http_add_header_num(http_info_t* info)
{
	http_logic_t* logic=http_get_logic_t(info);
        http_int_t* http_int=http_get_int_t(info);
        if ( logic->dir == 0){
                http_int->nhline_0++;
        }else{
                http_int->nhline_1++;
        }

	return 0;
}



int http_set_header_data(http_info_t* info,http_hline_t* item)
{
	assert(item);
	http_hctrl_t *header=http_get_header_t(info);
	char* data=info->data;
	http_str_t* name=&item->name;
	http_str_t* value=&item->value;
	
	char *begin=data+item->begin;
        char *split1=data+item->split;
        char *split2=0;
	if( split1 != begin){
		split2=split1+1;
	}else{
		split2=split1;
	}
        char *end=begin+item->len;
	int ret=0;
	
	if (begin > split1){
		return 0;
	}
	strlstrip(&begin,&split1);
	ret=data_set_buf(item->flag,&header->buf,&name->p,&name->len,split1-begin,begin,split1-begin,info);
	if( ret == NORMAL_ERR){
		ret=http_move_header(info);	
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
		
		ret=data_set_buf(item->flag,&header->buf,&name->p,&name->len,split1-begin,begin,split1-begin,info);
		CHECK_MEM(ret);
		if( ret ==NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
	if (split2 > end){
		return 0;
	}
	strlstrip(&split2,&end);
	//http_set_str(info,value,split2,end-split2);
	ret=data_set_buf(item->flag,&header->buf,&value->p,&value->len,end-split2,split2,end-split2,info);
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
		ret=http_move_header(info);	
		CHECK_MEM(ret);
		if( ret ==NORMAL_ERR){
			return NORMAL_ERR;
		}
		ret=data_set_buf(item->flag,&header->buf,&value->p,&value->len,end-split2,split2,end-split2,info);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
		
	http_add_header_num(info);
	return 0;
}

int http_set_max_header_line(http_info_t* info,int len)
{
	http_logic_t* logic=http_get_logic_t(info);
	http_int_t* http_int=http_get_int_t(info);
	u_int16_t* now=NULL;
	if ( logic->dir == 0){
		now=&http_int->max_hllen_0;	
	}else{
		now=&http_int->max_hllen_1;	
	}
	if( len>*now){
		*now=len;
	}
	return 0;
}
int http_add_header_len(http_info_t* info,int len)
{
	http_logic_t* logic=http_get_logic_t(info);
	http_int_t* http_int=http_get_int_t(info);
	if ( logic->dir == 0){
		http_int->hlen_0=http_int->hlen_0+len;	
	}else{
		http_int->hlen_1=http_int->hlen_1+len;	
	}
	return 0;
}

int http_parse_head(char* data,int data_len,int offset,http_info_t* info)
{
	int ret=0;
	int myret=0;
	int begin = 0;
	size_t len = 0;
	int end;
	int old_len=data_len;
	if( unlikely(data == NULL || info == NULL)){
		HTTP_MEM_ERR(info,data);
		return old_len;
	}
	http_hline_t* header=NULL;
//	info->http.header.cur=header;
	http_local_t* local=http_get_local_t(info);
	http_logic_t* logic=http_get_logic_t(info);
	http_hctrl_t* ctrl=http_get_header_t(info);
	//header=ctrl->cur;
#if debug_data
	memcpy(&mybuf[0]+mybuf_len,data,data_len);
	mybuf_len=mybuf_len+data_len;
#endif
	int status;
	int st;
	http_int_t* http_int=http_get_int_t(info);

	u_int16_t *hlen=NULL;
        if ( logic->dir == 0){
                hlen=&http_int->hlen_0;
        }else{
                hlen=&http_int->hlen_1;
        }
	int flag=0;


	http_set_status(info,HTTP_STATE_HEAD);
	while( data_len >0){
		status=local->sub_status;
		header=ctrl->cur;
		//header=http_get_cur_hline(info);
		switch (*data){
			case '\r': 
				st=http_hstate[status][1].state; 
				break; 
			case '\n': 
				st=http_hstate[status][2].state; 
				break; 
			case '%': 
				SET_NEED_URL_DECODE(flag); 
				st=http_hstate[status][0].state; 
				break; 
			case '&': 
				SET_NEED_HTML_DECODE(flag); 
				st=http_hstate[status][0].state; 
				break; 
			case '\\': 
				SET_NEED_ESCAPE_DECODE(flag); 
				st=http_hstate[status][0].state; 
				break; 
			case '+': 
				SET_NEED_URL_DECODE(flag); 
				st=http_hstate[status][0].state; 
				break; 
			case ':':
				if( header != NULL){
					if( header->split == 0){ 
						header->split=offset+ret+local->last_len; 
					}	 
				}
			default: 
				st=http_hstate[status][0].state; 
				break; 
		}
		
                local->sub_status=st;
                //local->sub_status=http_check_hstate(*data,local->sub_status,offset+ret,info);;


		switch( local->sub_status){
			case HTTP_HSTATE_CHAR1:
				if( header != NULL){
					header->flag=flag;
				}
				ctrl->cur= http_new_hline( info);
				if( ctrl->cur == NULL){
					return MEM_ERR;
				}	
				
				flag=0;
				header=ctrl->cur;
				begin=offset+ret;
				header->begin=begin+local->last_len;
				
				break;
			case HTTP_HSTATE_N1:
				if( header == NULL){
					break;
				}
				end=offset+ret-1+local->last_len;
				if( end > http_config.header_max){
					http_set_error(info,HTTP_HEADER_OVERSIZE);
				}
				myret=http_record_line(info,header->begin,end-header->begin,header->split);
				if( myret == NORMAL_ERR){
					return NORMAL_ERR;
				}
				http_set_max_header_line(info,end-header->begin);
				break;
			default:
				break;

		}
		if( local->sub_status== HTTP_HSTATE_N2){
			if( header != NULL){
				header->flag=flag;
			}
			http_set_status(info,HTTP_STATE_BODY);
			data++;
			data_len--;
			http_add_header_len(info,1);
#if debug_data
		//	http_int_t* http_int=http_get_int_t(info);
			if ( logic->dir == 0){
                		//memcpy(mybuf,info->data,http_int->hlen_0);
        		}else{
                		//memcpy(mybuf,info->data,http_int->hlen_1);
        		}
#endif 

			info->data=session_get_origin_header(info->ssn, &len, logic->dir);
			/*char buf[4096];
			memcpy(buf,info->data,len);
			buf[len]='\0';
			printf(" get data(%s)\n",buf);*/
			if( info->data == NULL){
				return NORMAL_ERR;
			}	
			myret=http_parse_line(info);
			CHECK_MEM(myret);
			CHECK_VER(myret);
			if( myret == NORMAL_ERR){
				return NORMAL_ERR;
			}
			if( data_len == 0 && http_get_int(info,HTTP_INT_CLEN) == 0 && get_transfer_encoding(info)==0 ){
				if ( logic->is_closed == HTTP_LOGIC_FALSE){
					http_set_status(info,HTTP_STATE_NONE);
				}
			}

			ret++;
			break;
		}
		(*hlen)++;
		data++;
		data_len--;
		ret++;
	}
	
	local->last_len=local->last_len+old_len;
	
	return ret;
}






int http_init_local_after_request(http_info_t* info)
{
	http_local_t* local=http_get_local_t(info);
	local->sub_status= HTTP_HSTATE_CHAR0;
	local->tenc_0=0;
	local->tenc_1=0;
	local->unrcv_len_0=0;
	local->unrcv_len_1=0;
	local->cache_buflen=0;
	local->unrcv_cklen=0;
	local->chunk_flag=HTTP_CHUNK_SIZE;
	local->form_status=HTTP_FORM_BEGIN;
        local->form_flag=HTTP_FORM_ARG;
        local->data_flag=0;
        local->subject_flag=0;
        local->cert_flag=0;
        local->realip_flag=0;
        local->abs_url_flag=0;
        local->clen_isset_0= 0;
        local->clen_isset_1= 0;
        local->myform_label_state= LABEL_STATE_NULL;
        local->myform_label_num= 0;
	local->form_header_state = 0;
        local->boundary_cnt= 0;
        local->last_len= 0;
        local->null_file= 0;
        local->have_8341= 0;
        local->force_decode= 0;
	
        //local->cfg_real_flag;
        //char org_header[48];
        //int org_header_len;
	return 0;
}


int http_init_local(http_info_t* info)
{
	http_init_local_after_request(info);
	return 0;
}
int http_init_logic_after_request(http_info_t* info)
{
	http_logic_t* logic=http_get_logic_t(info);

	//logic->state_0 = HTTP_STATE_INIT;
	logic->state_1 = HTTP_STATE_INIT;
        logic->dir=0;

        logic->is_chunked= HTTP_LOGIC_FALSE;
        logic->is_rpc= HTTP_LOGIC_FALSE;
        logic->is_dropargs = HTTP_LOGIC_FALSE;
        logic->is_ver_11 = HTTP_LOGIC_FALSE;
        logic->version_1 = HTTP_LOGIC_FALSE;
        logic->is_gzip = HTTP_LOGIC_FALSE;
        logic->is_only_gzip = HTTP_LOGIC_FALSE;
        logic->is_cenc= HTTP_LOGIC_FALSE;
        logic->is_closed=HTTP_LOGIC_FALSE;

	return 0;
}
int http_init_logic(http_info_t* info)
{
	http_logic_t* logic=http_get_logic_t(info);

	logic->method=HTTP_METHOD_NONE;  // keep
	logic->version_0=HTTP_VER_NA; //keep
        logic->is_wafrdt=HTTP_LOGIC_FALSE; //keep
        logic->is_wafsid=HTTP_LOGIC_FALSE; //keep
        logic->ctype_0=HTTP_TYPE_NA;
        logic->ctype_1=HTTP_TYPE_NA;
        logic->state_0=HTTP_STATE_INIT;
        logic->state_1=HTTP_STATE_INIT;

	http_init_logic_after_request(info);

	return 0;
}
int http_init_int_after_request(http_info_t* info)
{
	http_int_t* http_int=http_get_int_t(info);
	http_int->retcode=0;
	http_int->ncookie_1=0;
	http_int->nhline_1=0;
	http_int->max_hllen_1=0;
	http_int->blen_1=0;
	http_int->clen_1=0;
	/*http_int_t* http_int=http_get_int_t(info);
	http_int->range=0;
	http_int->retcode=0;
	http_int->n_cookie_0=0;
	http_int->n_cookie_1=0;
	http_int->n_argument=0;
	http_int->n_headline_0=0;
	http_int->n_headline_1=0;
	http_int->max_header_line_len_0=0;
	http_int->max_header_line_len_1=0;
	http_int->body_len_0=0;
	http_int->body_len_1=0;
	http_int->content_len_0=0;
	http_int->content_len_1=0;
	http_int->header_len_0=0;*/
	return 0;
}

int http_init_buf( http_info_t* info)
{
	if( info->mp != NULL){
		printf(" fatal error mp is not null\n");
	}
	info->mp=NULL;
	int ret=0;
	ret = apr_pool_create(&info->mp, NULL);
        if (ret != APR_SUCCESS) {
                printf("create pool failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }

	apr_allocator_t *pa = apr_pool_allocator_get(info->mp);
	if (pa) {
		apr_allocator_max_free_set(pa, 1);
	}

	info->http=http_malloc(info,sizeof(http_para_t));
	HTTP_MEM_ERR(info,info->http);
	
	info->http->arg.buf.buf=http_malloc(info, 4096);
	HTTP_MEM_ERR(info,info->http->arg.buf.buf);

	http_hctrl_t* header=&info->http->header_0;
	header->array=apr_array_make(info->mp,32 , sizeof(http_hline_t *));
	if (!header->array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	//header->buf.buf=http_malloc(info, 4096);
	header->buf.buf=NULL;
	header->buf.current=0;
	header->extern_flag=0;
	header->buf.max_len=4095;
	header->cur=NULL;
	header->num=0;

	header=&info->http->header_1;
	header->array=apr_array_make(info->mp, 32, sizeof(http_hline_t *));
	if (!header->array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	header->buf.buf=NULL;
	//header->buf.buf=http_malloc(info, 4096);
	header->buf.current=0;
	header->extern_flag=0;
	header->buf.max_len=4095;
	header->cur=NULL;
	header->num=0;

	info->http->file.array=apr_array_make(info->mp, 16, sizeof(http_file_t *));
	if (!info->http->file.array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	info->http->file.cur=NULL;
	info->http->file.num=0;
	info->http->file.new_num=0;
	
	info->http->av.array=apr_array_make(info->mp, 16, sizeof(http_av_t *));
	if (!info->http->av.array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	info->http->av.cur=NULL;
	info->http->av.num=0;
	
	info->http->arg.array=apr_array_make(info->mp, 48, sizeof(http_arg_t *));
	if (!info->http->arg.array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	//info->http->arg.buf.buf=http_malloc(info, 4096);
	//HTTP_MEM_ERR(info,info->http->arg.buf.buf);
	info->http->arg.buf.current=0;
	info->http->arg.buf.max_len=4095;
	info->http->arg.cur=NULL;
	info->http->arg.extern_flag=0;
	info->http->arg.num=0;
	info->http->arg.new_num=0;

	info->http->cookie_0.array=apr_array_make(info->mp, 48, sizeof(http_cookie_t *));
	if (!info->http->cookie_0.array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	info->http->cookie_0.cur=NULL;
	info->http->cookie_0.num=0;

	info->http->cookie_1.array=apr_array_make(info->mp, 48, sizeof(http_cookie_t *));
	if (!info->http->cookie_1.array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	info->http->cookie_1.cur=NULL;
	info->http->cookie_1.num=0;


	info->http->amf.array=apr_array_make(info->mp, 4, sizeof(http_amf_t *));
	if (!info->http->amf.array) {
                printf("create array failed\n");
		info->mem_error=1;
                goto OUT_FREE;
        }
	info->http->amf.buf.buf=NULL;
	info->http->amf.buf.current=0;
	info->http->amf.buf.max_len=http_config.amf_cache_size;
	info->http->amf.cur=NULL;
	info->http->amf.num=0;
	
	return 0;
OUT_FREE:
        if (info->mp){
                apr_pool_destroy(info->mp);
	}
	return MEM_ERR;
}


int http_init(http_info_t* info,void* sess)
{
	

	
	int ret=0;
//	HTTP_CHECK_ERR(sess);
	if ( info->init_flag ==1 ){
		return 0;
	}
	info->ssn = sess;
	//memset(info,0x00,sizeof(http_info_t));
	if ( info->http != NULL ){
		http_int_t* http_int=http_get_int_t(info);
		assert(http_int);
		if ( http_int->retcode == 100){
			return 0;
		}
	}
	if( http_config.decode_flag == STATUS_ENABLE){
		http_config.decode_flag=CIC_DECODE;
	}else{
		http_config.decode_flag=ONLY_ONE_DECODE;
	}
	http_clean(info);
	memcpy(&info->cfg,&http_config,sizeof(http_config_t));
	ret=http_init_buf( info);
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
		return NORMAL_ERR;
	}
	http_init_local(info);
	http_init_logic(info);
	http_init_int(info);
	http_init_str(info);
	info->init_flag = 1;
	info->mem_error = 0;
	info->ver_error = 0;
	info->error_line = 0;
	info->error = 0;
	
	//http_init_array(info);
        return 0;
}


int http_check_after_parse(char* data,int data_len,http_info_t* info,int dir)
{
	http_logic_t* logic=http_get_logic_t(info);
	if ( dir == 1){
		if ( logic->state_1 == HTTP_STATE_BODY && logic->method == HTTP_METHOD_HEAD){	
			logic->state_1 =	HTTP_STATE_NONE;
		}
	}
	return 0;
}

int  check_error(http_info_t* info)	
{
	int myret=0;
	http_local_t* local=http_get_local_t(info);
	if( local->header_buf_error == 1){
		printf(" header\n");
		myret=myret|HTTP_HEADER_OVERSIZE;
	}
	if( local->args_buf_error == 1){
		printf(" args\n");
		myret=myret|HTTP_ARGBUF_OVERSIZE;
	}
	if( local->file_num_error == 1){
		printf(" file\n");
		myret=myret|HTTP_FILE_NUM_OVERSIZE;
	}
	return myret;
		
}

int http_is_critical_error(http_info_t* info)
{
	HTTP_FUNC_0(info);
	if( info->mem_error==1 || info->ver_error==1){
		return 1;
	}
	return 0;
}
int http_get_normal_error(http_info_t* info)
{
	HTTP_FUNC_0(info);
	if ( info->error_line != 0){
		return info->error_line;
	}
	return 0;
}
int http_get_limit_error(http_info_t* info)
{
	HTTP_FUNC_0(info);
	http_local_t* local=http_get_local_t(info);
	assert(local);
	int myret=0;
	if( local->header_buf_error == 1){
		myret=myret|HTTP_HEADER_OVERSIZE;
	}
	if( local->args_buf_error == 1){
		myret=myret|HTTP_ARGBUF_OVERSIZE;
	}
	if( local->file_num_error == 1){
		myret=myret|HTTP_FILE_NUM_OVERSIZE;
	}
	return myret;
		
}
int http_parse(char* data,int data_len,http_info_t* info,int dir)
{
	if( unlikely(data == NULL)|| unlikely(info==NULL) || data_len <0) {

		printf(" data error %d\n",data_len);
		return -2;	
	}
	/*char buf[8192];
	memcpy(buf,data,data_len);
	buf[data_len]='\0';
	printf(" http%d(%s)\n",data_len,buf);*/
	int ret=0;
	int offset=0;
	int raw_len=data_len;
	info->data=data;
	volatile int status;
	
	ret=http_check_before_parse(data,data_len,info,dir);
	if( ret == NORMAL_ERR){
		goto out;
	}
	http_local_t* local=http_get_local_t(info);
	assert(local);
	local->header_buf_error=0;
	local->args_buf_error=0;
	local->amf_error=0;
	local->file_num_error=0;
	info->error=0;

	if( info->mem_error ==1 || info->ver_error ==1){
		goto out;
	}
	while( data_len>0){

		status=http_get_logic(info,HTTP_LOGIC_STATE);	

		if ( status == HTTP_STATE_NONE && data_len>0){
			info->ver_error=1;	
			goto out;
		}
			
		ret=http_mstate[status].func(data+offset,data_len,offset,info);
		if( ret == NORMAL_ERR){
			goto out;
		}

		
		if( info->mem_error!=0 || info->ver_error!=0){
			ret =NORMAL_ERR;
			goto out;
		}
		if( ret <0){
			goto out;
		}
		offset=offset+ret;
		data_len=data_len-ret;
		http_check_after_parse(data,data_len,info,dir);
	}

out:
	//http_dump_header(info);
	//http_dump_logic(info);
	if ( raw_len >0){
		info->init_flag =0;
	}
	if( info->mem_error!=0 || info->ver_error!=0){
		printf(" http 1\n");
		return -1;
	}
	if ( check_error(info) != 0){
		printf(" http 2\n");
		return -2;
	}
	return 0;	
}



#include "http_decode.h"
int myset_data(http_info_t* info,int idx,char* data,int data_len,int flag)
{
#if 0
	if( GET_NEED_DECODE(flag) == 0){
		http_save_str(info,idx,data,data_len);
                return 0;
        }
        char tmp_buf[MAX_DECODE_LEN]={0};
        int len=0;

        len=http_url_decode(tmp_buf,sizeof(tmp_buf),data,data_len,flag,info->http_local.global_decode_flag);
	http_save_str(info,idx,tmp_buf,len);
	return 0;
#endif
	return 0;

}



int http_set_arg(char* name,int name_len,char* value,int value_len,http_info_t* info,http_arg_t* item)
{
	assert(item);
	http_argctrl_t *arg=http_get_arg_t(info);
	http_int_t *http_int=http_get_int_t(info);
	http_str_t* aname=&item->name;
	http_str_t* avalue=&item->value;
	int ret=0;

	http_int->allarg_len=http_int->allarg_len+name_len+value_len;
	//http_set_str(info,arg_name,name,name_len);
	ret=http_append_data_buf(&arg->buf,&aname->p,&aname->len,name_len,name,name_len);
	CHECK_MEM(ret);
	if( ret==NORMAL_ERR){
		ret=http_move_args(info);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
		ret=http_append_data_buf(&arg->buf,&aname->p,&aname->len,name_len,name,name_len);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
	//http_set_str(info,arg_value,NULL,value_len);
	ret=http_append_data_buf(&arg->buf,&avalue->p,&avalue->len,value_len,value,value_len);
	if( ret==NORMAL_ERR){
		ret=http_move_args(info);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
		ret=http_append_data_buf(&arg->buf,&avalue->p,&avalue->len,value_len,value,value_len);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}
	
	return 0;
}

int get_as_param(http_info_t* info)
{
	http_local_t* local=http_get_local_t(info);
	if( local->have_8341 == 1){
		return 0;
	}
	http_protocol_t* pro=http_get_protocol_t(info);

        int i=0;
        int cnt=http_get_arg_cnt(info);
	int ret=0;
        http_arg_t* arg=NULL;
        for (i = 0; i < cnt; i++) {
                arg = http_get_arg(info,i);
		assert(arg);
		if( strcmp(arg->name.p,HTTP_PARAM_NAME) == 0){
			ret=http_set_str(info,&pro->js_arg,arg->value.p,arg->value.len);
			CHECK_MEM(ret);
			local->have_8341=1;
			break;
	
		}
        }
        return 0;
	
}
int _http_parse_args(http_info_t* info,char* data,int data_len,int in_url)
{
	assert(data);
	char* begin=data;
	int len=data_len;

	int name_len=0;
	int value_len=0;
	int* myflag=NULL;
	int* mylen=&name_len;
	char* tmp_data=NULL;
	int tmp_len=0;
	
	int ret=0;
	http_int_t* http_int=http_get_int_t(info);

	http_argctrl_t* arg=http_get_arg_t(info);

	if( arg->cur == NULL){
		arg->cur=http_array_new_arg( info);
		if( arg->cur == NULL){
			return MEM_ERR;
		}
		ret=http_set_arg(NULL,0,NULL,0,info,arg->cur);
		CHECK_MEM(ret);
	}
	myflag=&arg->cur->name_flag;
	while(len>0){
		switch(*begin){
			case '%':
                        case '+':
                                SET_NEED_URL_DECODE(*myflag);
				if( tmp_data== NULL){
					tmp_data=begin;
				}
				tmp_len++;
				//http_append_arg(begin,1,info,arg->cur);	
				if( in_url ){
					http_int->urlarg_len=http_int->urlarg_len+1;
				}
                                break;
                        case '\\':
                                SET_NEED_ESCAPE_DECODE(*myflag);
				if( tmp_data== NULL){
					tmp_data=begin;
				}
				tmp_len++;
				//http_append_arg(begin,1,info,arg->cur);	
				if( in_url ){
					http_int->urlarg_len=http_int->urlarg_len+1;
				}
				break;
                        case '&':
				ret=http_append_arg(tmp_data,tmp_len,info,arg->cur);	
				CHECK_MEM(ret);
				tmp_data=NULL;
				tmp_len=0;
				ret=http_append_arg("\0",1,info,arg->cur);	
				if( in_url){
					http_int->nurlarg++;
				}
				CHECK_MEM(ret);
				arg->cur->complete_flag=1;
				arg->cur=http_array_new_arg( info);
				if( arg->cur == NULL){
					return MEM_ERR;
				}
				name_len=0;
				value_len=0;
				myflag=&arg->cur->name_flag;
				mylen=&name_len;
				arg->cur->arg_flag=0;
				ret=http_set_arg(NULL,0,NULL,0,info,arg->cur);
				CHECK_MEM(ret);
                                break;
			case '=':
				if(  arg->cur->arg_flag != 1){
					ret=http_append_arg(tmp_data,tmp_len,info,arg->cur);	
					CHECK_MEM(ret);
					tmp_data=NULL;
					tmp_len=0;
					ret=http_append_arg("\0",1,info,arg->cur);	
					CHECK_MEM(ret);
					myflag=&arg->cur->value_flag;
					mylen=&value_len;
					arg->cur->arg_flag=1;
				}else{
					tmp_len++;
				}
				break;
			default:
				if( tmp_data== NULL){
					tmp_data=begin;
				}
				tmp_len++;
				//http_append_arg(begin,1,info,arg->cur);	
				if( in_url ){
					http_int->urlarg_len=http_int->urlarg_len+1;
				}
				break;
		}
		begin++;
		len--;
		
	} 	
	
	ret=http_append_arg(tmp_data,tmp_len,info,arg->cur);	
	CHECK_MEM(ret);
	if( in_url == 0&& http_get_logic(info,HTTP_LOGIC_STATE)==HTTP_STATE_NONE){
		ret=http_append_arg("\0",1,info,arg->cur);	
		CHECK_MEM(ret);
		arg->cur->complete_flag=1;
	}
	if( in_url){
		ret=http_append_arg("\0",1,info,arg->cur);	
		http_int->nurlarg++;
		CHECK_MEM(ret);
		arg->cur->complete_flag=1;
	}
	if( in_url){
		ret=get_as_param(info);
		CHECK_MEM(ret);
	}
	
	return 0;
}




int parse_url_arg(http_info_t* info)
{
#if 0
	char* begin=NULL;
	int len=0;
	begin=http_get_str(info,HTTP_STR_RAW_URL)+http_get_strlen(info,HTTP_STR_RAW_URL)+1;
	len= http_get_strlen(info,HTTP_STR_RAW_URL)-http_get_strlen(info,HTTP_STR_RAW_URL)-1;
	int i=0;

	char* name=NULL;
	char* value=NULL;
	int name_len=0;
	int value_len=0;
	int name_flag=0;
	int value_flag=0;
	int* myflag=&name_flag;
	int* mylen=&name_len;
	

	name=&begin[i];
	for(i=0;i<len;i++){
		switch(begin[i]){
			case '%':
                        case '+':
                                SET_NEED_URL_DECODE(*myflag);
				*mylen=*mylen+1;
                                break;
                        case '\\':
				*mylen=*mylen+1;
                                SET_NEED_ESCAPE_DECODE(*myflag);
				break;
                        case '&':
				http_save_arg(info,name,name_len,name_flag,value,value_len,value_flag);
				name=&begin[i+1];
				name_len=0;
				value_len=0;
				myflag=&name_flag;
				mylen=&name_len;
                                break;
			case '=':
				value=&begin[i+1];
				myflag=&value_flag;
				mylen=&value_len;
				break;
			default:
				*mylen=*mylen+1;
				break;
		}
		
	} 	
	if( name_len >0 || value_len>0){
		http_save_arg(info,name,name_len,name_flag,value,value_len,value_flag);
	}
#endif
	return 0;
}


int http_add_body_len(http_info_t* info,int len)
{
	http_logic_t* logic=http_get_logic_t(info);
        http_int_t* http_int=http_get_int_t(info);
        if ( logic->dir == 0){
                http_int->blen_0= http_int->blen_0+len;
        }else{
                http_int->blen_1= http_int->blen_1+len;
        }

	return 0;
}
int http_init_common_buf(http_combuf_t* buf,int len,http_info_t* info)
{
	buf->buf=http_malloc(info, len);
	
	HTTP_MEM_ERR(info,buf->buf);
        buf->current=0;
        buf->max_len=len-1;
	return 0;
}

int 
http_copy_data_buf(http_combuf_t* buf,char** data,int *data_len,int max_len,char* value,int value_len,http_info_t* info)
{
        int offset=0;
	char* mydata=value;
	int mylen=value_len;
        if( buf == NULL || data == NULL || data_len == NULL|| value == NULL || data==NULL|| buf->buf== NULL){
                return 0;
        }
        if( value_len > max_len){
                goto err;
        }
        if( buf->current+value_len>= buf->max_len){
                goto err;
        }
        offset=buf->current;

        memlcpy(&buf->buf[buf->current],buf->max_len-buf->current,mydata,mylen);
	
        *data=&buf->buf[buf->current];
        *data_len=mylen;

       	buf->current=buf->current+mylen;
        buf->buf[buf->current]='\0';
       	buf->current++;
        return 0;
err:
        *data="\0";
        *data_len=0;
        return NORMAL_ERR;
}

int http_set_error(http_info_t* info,int error)
{
        http_local_t* local=http_get_local_t(info);
	switch( error){
		case HTTP_HEADER_OVERSIZE:
        		local->header_buf_error = 1;
			break;
		case HTTP_ARGBUF_OVERSIZE:
        		local->args_buf_error = 1;
			break;
                case HTTP_FILE_NUM_OVERSIZE:
        		local->file_num_error = 1;
			break;
		default:
			break;
	}
	info->error=info->error|error;
	return 0;
}
int http_get_error(http_info_t* info)
{
	HTTP_FUNC_0(info);
	return info->error;
}

int http_move_header(http_info_t* info)
{
	http_hctrl_t *ctl=http_get_header_t(info);
	if( ctl->extern_flag ==1){
		//http_set_error(info,HTTP_HEADER_OVERSIZE);
		return 0;
	}
	int i=0;
        apr_array_header_t *arr=ctl->array;
        http_hline_t* header=NULL;
	int cnt=arr->nelts;
	http_combuf_t buf;
	memset(&buf,0x00,sizeof(buf));
	ctl->extern_flag=1;

	int ret=http_init_common_buf(&buf,http_config.header_max,info);
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
		return NORMAL_ERR;
	}
        for (i = 1; i < cnt; i++) {
                header = http_get_hline(info,i);
		assert(header);
		ret=http_copy_data_buf(&buf,&header->name.p,&header->name.len,header->name.len,header->name.p,header->name.len,info);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
		ret=http_copy_data_buf(&buf,&header->value.p,&header->value.len,header->value.len,header->value.p,header->value.len,info);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
        }
	memcpy(&ctl->buf,&buf,sizeof(buf));

	return 0;
	
}

int http_move_args(http_info_t* info)
{
        int i=0;
        http_argctrl_t *ctl=http_get_arg_t(info);
	if( ctl->extern_flag ==1){
		http_set_error(info,HTTP_ARGBUF_OVERSIZE);
		return 0;
	}
	ctl->extern_flag=1;

        apr_array_header_t *arr=ctl->array;
        http_arg_t* arg=NULL;
	http_combuf_t buf;
	memset(&buf,0x00,sizeof(buf));
	int ret=http_init_common_buf(&buf,http_config.args_max,info);
	CHECK_MEM(ret);
        for (i = 0; i < arr->nelts; i++) {
                arg = http_get_arg(info,i);
		assert(arg);
		ret=http_copy_data_buf(&buf,&arg->name.p,&arg->name.len,arg->name.len,arg->name.p,arg->name.len,info);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
		if( arg->value.len<=0){
			continue;
		}
		ret=http_copy_data_buf(&buf,&arg->value.p,&arg->value.len,arg->value.len,arg->value.p,arg->value.len,info);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}

        }
	memcpy(&ctl->buf,&buf,sizeof(buf));
        return 0;

}
int http_set_file(char* name,int name_len,char* value,int value_len,http_info_t* info,http_file_t* item)
{
	if( item == NULL){
		return 0;
	}

	http_str_t* fname=&item->name;
	if( http_get_file_cnt(info)>= http_config.upfile_count){
		http_set_error(info,HTTP_FILE_NUM_OVERSIZE);
		return NORMAL_ERR;
	}
	item->raw_file_len=value_len;
	int ret=http_set_str(info,fname,name,name_len);
	CHECK_MEM(ret);

	//http_set_str(info,fvalue,NULL,value_len);
	return 0;
}
int http_append_file(char* value,int value_len,http_info_t* info,http_file_t* item)
{
	if( item == NULL){
		return 0;
	}
	if( http_get_file_cnt(info)>= http_config.upfile_count){
		http_set_error(info,HTTP_FILE_NUM_OVERSIZE);
		return NORMAL_ERR;
	}
	http_str_t* fvalue=&item->value;
	//item->raw_file_len=item->raw_file_len+value_len;
	assert(fvalue);
	if( fvalue->len+value_len >HTTP_FILE_MAX_LEN){
		item->complete_flag=1;
		return 0;
	}

	return http_append_str(info,fvalue,value,value_len);
}
http_file_t* http_array_new_file(http_info_t* info)
{
	http_fctrl_t* file=http_get_file_t(info);
	if( http_get_file_cnt(info)>= http_config.upfile_count){
		http_set_error(info,HTTP_FILE_NUM_OVERSIZE);
		return NULL;
	}
	http_file_t* item=http_malloc(info,sizeof(http_file_t));
	HTTP_MEM_NULL(info,item);
	memset(item,0x00,sizeof(http_file_t));
	file->new_num++;
	*(http_file_t**)apr_array_push(file->array)=item;
	return item;
}
int http_set_av(char* name,int name_len,char* value,int value_len,http_info_t* info,http_av_t* item)
{
	HTTP_CHECK_ERR(item);
	http_str_t* fname=&item->name;
	http_str_t* fvalue=&item->value;

	int ret=0;
	ret=http_set_str(info,fname,name,name_len);
	CHECK_MEM(ret);
	ret=http_set_str(info,fvalue,value,value_len);
	CHECK_MEM(ret);
	return 0;
}
http_av_t* http_array_new_av(http_info_t* info)
{
	http_avctrl_t* av=http_get_av_t(info);
	http_av_t* item=http_malloc(info,sizeof(http_av_t));
	HTTP_MEM_NULL(info,item);
	memset(item,0x00,sizeof(http_av_t));

	*(http_av_t**)apr_array_push(av->array)=item;
	return item;
} 

http_amf_t * 
http_array_new_amf(http_info_t* info)
{
	http_amfctrl_t* amf=http_get_amf_t(info);
	http_amf_t* item=http_malloc(info,sizeof(http_amf_t));
	HTTP_MEM_NULL(info,item);
	memset(item,0x00,sizeof(http_amf_t));
	*(http_amf_t**)apr_array_push(amf->array)=item;
	return item;
}

int 
http_set_amf(char* name,int name_len,char* value,int value_len, 
	     char* class_name,int class_name_len,http_info_t* info)
{
	if( name_len == 0 && value_len == 0){
		return 0;
	}
	http_amf_t* cur=http_array_new_amf(info);
	HTTP_MEM_ERR(info,cur);
 	http_amfctrl_t *amf=http_get_amf_t(info);

        http_str_t* aname=&cur->name;
        http_str_t* avalue=&cur->value;
        http_str_t* aclass=&cur->class_name;

	int ret=0;
	if( amf->buf.buf== NULL){
		amf->buf.buf=http_malloc(info,http_config.amf_cache_size);
		HTTP_MEM_ERR(info,amf->buf.buf);
	}
	if( name_len >0){
        	ret=http_append_data_buf(&amf->buf,&aname->p,&aname->len,name_len,name,name_len);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
		ret=http_append_data_buf(&amf->buf,&aname->p,&aname->len,name_len,"\0",1);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
			return NORMAL_ERR;
		}
	}

	if( value_len >0){
        	ret=http_append_data_buf(&amf->buf,&avalue->p,&avalue->len,value_len,value,value_len);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
		}

	        ret=http_append_data_buf(&amf->buf,&avalue->p,&avalue->len,value_len,"\0",1);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
		}
	}


	if( class_name_len >0){
        	ret=http_append_data_buf(&amf->buf,&aclass->p,&aclass->len,class_name_len,class_name,class_name_len);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
		}
       		ret=http_append_data_buf(&amf->buf,&aclass->p,&aclass->len,class_name_len,"\0",1);
		CHECK_MEM(ret);
		if( ret == NORMAL_ERR){
		}
	}


	//http_set_str(info,&cur->name,name,name_len);
	//http_set_str(info,&cur->value,value,value_len);
	//http_set_str(info,&cur->class_name,class_name,class_name_len);

	return 0;
}

int 
http_set_protocol_str(http_info_t* info,int index,char* data,int len)
{
	http_protocol_t* protocol=http_get_protocol_t(info);;
	http_str_t* item=NULL;
	int ret;
	switch(index){
		case HTTP_STR_HOST:
			item=&protocol->raw_host;
			ret=http_set_str(info,item,data,len);
			CHECK_MEM(ret);
			break;
		case HTTP_STR_BOUNDARY:
			item=&protocol->boundary;
			ret=http_set_str(info,item,data,len);
			CHECK_MEM(ret);
		default:
			break;
	}
	return 0;
}

typedef struct my_item {
        int     type;
        char    *name;
} my_item_t;


int _apr_array_test(void)
{
        apr_pool_t *mp = NULL;
        apr_array_header_t *arr;
        my_item_t *item;
        int ret = 0;
        int i;

        apr_initialize();

        ret = apr_pool_create(&mp, NULL);
        if (ret != APR_SUCCESS) {
                printf("create pool failed\n");
                goto OUT_FREE;
        }

        arr = apr_array_make(mp, 4, sizeof(my_item_t *));
        if (!arr) {
                printf("create array failed\n");
                goto OUT_FREE;
        }

        for (i = 0; i < 10; i++) {
                item = apr_palloc(mp, sizeof(my_item_t));
                if (!item) {
                        printf("malloc memory from pool failed\n");
                        goto OUT_FREE;
                }
                item->type = i + 1;
                item->name = apr_palloc(mp, 10);
                snprintf(item->name, 10, "ljsb-%d", i);
                item->name[9] = 0;
                *(my_item_t **)apr_array_push(arr) = item;
        }

        for (i = 0; i < arr->nelts; i++) {
                item = ((my_item_t **)arr->elts)[i];
                printf("item[%d], name %s\n", item->type, item->name);
        }

OUT_FREE:
        if (mp)
                apr_pool_destroy(mp);

        apr_terminate();

        return ret;
}

int recv_content_len(char *data, int data_len,http_info_t* info)
{
	assert(data);
	http_local_t* local=http_get_local_t(info);
	http_logic_t* logic=http_get_logic_t(info);
	http_add_un_recv_len(info,0-data_len);
	http_add_body_len(info,data_len);
	int len=0;
	if( logic->dir == 0){
		len=local->unrcv_len_0;
	}else{
		len=local->unrcv_len_1;
	}
	if( len<=0){
		if ( logic->is_closed == HTTP_LOGIC_FALSE){
			http_set_status(info,HTTP_STATE_NONE);
		}
	}
	
	return 0;
}

int 
http_wait_label_str(char* buf,int buf_len,char* data,int data_len, 
	      http_info_t* info,char* str,int str_len)
{
        int len=0;
        http_local_t* local=http_get_local_t(info);

	/*switch(*data){
		case '-':
			if( local->form_label_last ==1){
				local->form_label_state++;
			}
			local->form_label_last=1;
			break;
		default:
			local->form_label_last=0;
			break;
	}
	if( local->form_label_state+1<local->boundary_cnt){
                return NORMAL_ERR;
	}*/
	int ret=NORMAL_ERR;
	len=strlcpy2(local->cache_buf+local->cache_buflen,
                        sizeof(local->cache_buf)-local->cache_buflen,
                        data,data_len);
        local->cache_buflen=local->cache_buflen+len;

	if( local->cache_buflen == str_len-local->boundary_cnt){
		if( memcmp(local->cache_buf,str+local->boundary_cnt,str_len-local->boundary_cnt) == 0){
			ret=strlcpy2(buf,buf_len,local->cache_buf,local->cache_buflen);
		}else{
			ret=NORMAL_ERR;
		}
        	local->cache_buflen=0;
		local->myform_label_state=LABEL_STATE_NULL;
	}
	return ret;

}
int http_wait_line_str(char* buf,int buf_len,char* data,int data_len, 
	      http_info_t* info,char* str,int str_len)
{
	int ret=NORMAL_ERR;
	int len=0;
        http_local_t* local=http_get_local_t(info);
        len=strlcpy2(local->cache_buf+local->cache_buflen,
                        sizeof(local->cache_buf)-local->cache_buflen,
                        data,data_len);
        local->cache_buflen=local->cache_buflen+len;
	switch(*data){
		case '\r':
			if( local->form_header_state == 0){	
				local->form_header_state=1;
			}else if( local->form_header_state ==2){
				local->form_header_state=3;
			}else{
			}
			break;
		case '\n':
			if( local->form_header_state == 1){	
				local->form_header_state=2;
			}else if( local->form_header_state ==3){
				local->form_header_state=4;
				ret=local->cache_buflen;	
        			local->cache_buflen=0;
				local->form_header_state=0;
			}else{
			}
			break;
	
		default:
			local->form_header_state=0;
			ret=NORMAL_ERR;
			break;
			
	}
	return ret;

}
int 
http_get_form_lable_value(char* data,int len,char* label,int label_len,
		char* name,int* name_len,int max_len)
{
	char* begin=NULL;
	char* end=NULL;
	begin=strlstr(data,len,label,label_len);
	if( begin == NULL){
		return NORMAL_ERR;
	}
	begin=begin+label_len;
	end=strlstr(begin,data+len-begin,"\"",1);
	if( end== NULL){
		return NORMAL_ERR ;
	}
	*name_len=strlcpy2(name,max_len,begin,end-begin);
	return 0;

}
int 
http_get_form_lable_value2(char* data,int len,char* label,int label_len,
		char** name,int* name_len,int max_len)
{
	char* begin=NULL;
	char* end=NULL;
	begin=strlstr(data,len,label,label_len);
	if( begin == NULL){
		return NORMAL_ERR;
	}
	begin=begin+label_len;
	end=strlstr(begin,data+len-begin,"\"",1);
	if( end== NULL){
		return NORMAL_ERR;
	}
	assert(name);
	assert(name_len);
	*name=begin;
	*name_len=end-begin;
	return 0;

}

int http_parse_form(char* data,int len,http_info_t* info)
{
	HTTP_CHECK_ERR(data);
	char* mydata=data;
	int mylen=len;
	int ret=0;
	int ret_len=0;
	char buf[256]={0};
	char label[256]={0};
	char file_name[512];
	int file_name_len=0;
	char name[512];
	int name_len=0;
	int label_len=0;
	char* name_label="name=\"";
	int name_label_len=strlen("name=\"");
	char* file_label="filename=\"";
	int file_label_len=strlen("filename=\"");

	http_local_t* local=http_get_local_t(info);	
	http_protocol_t* pro=http_get_protocol_t(info);	

	snprintf(label,sizeof(label),"--%s",pro->boundary.p);
	label_len=strlen(label);

	http_fctrl_t* file=http_get_file_t(info);
	http_argctrl_t* arg=http_get_arg_t(info);
	int myret=0;
	while(mylen>0){
		if( local->form_status == HTTP_FORM_BEGIN){
			switch(*mydata){
				case '-':
					local->myform_label_num++;
					if( local->myform_label_num==local->boundary_cnt){
						local->myform_label_state=LABEL_STATE_BEGIN;
					}
					break;
				default:
					if( local->myform_label_state==LABEL_STATE_BEGIN){
						local->myform_label_state=LABEL_STATE_RUN;
					}else {
					}
					if( local->myform_label_state!=LABEL_STATE_RUN){
						local->myform_label_state=LABEL_STATE_NULL;
						local->myform_label_num=0;
					}
					local->myform_label_num=0;

					break;
			}
			if( local->myform_label_state == LABEL_STATE_RUN){
				ret=http_wait_label_str(buf,sizeof(buf),mydata,1,info,label,label_len);
			}else{
                		ret=NORMAL_ERR;
			}
	
			mydata++;
			mylen--;
			if( ret ==NORMAL_ERR){
			}else{
				local->form_status =HTTP_FORM_HEADER;
				local->form_flag =HTTP_FORM_ARG;
			}
			continue;
		}else if( local->form_status == HTTP_FORM_HEADER){
			ret_len=http_wait_line_str(buf,sizeof(buf),mydata,1,info,"\r\n\r\n",4);
			mydata++;
			mylen--;
			if( ret_len ==NORMAL_ERR){
			}else{
				local->form_status =HTTP_FORM_BODY;
				ret=http_get_form_lable_value(local->cache_buf,ret_len,
						file_label,file_label_len,file_name,&file_name_len,sizeof(file_name));
				if( ret == 0){
					if( file->cur != NULL){
						//file->cur->value.p[file->cur->value.len-2]='\0';
					}
					if( file_name_len>0){

						local->null_file=0;
						file->cur=http_array_new_file( info);
						if ( file ->cur == NULL){
							//	return MEM_ERR;
						}
				 		if ( file->cur != NULL ){
							file->cur->complete_flag=0;
						}
						myret=http_set_file(file_name,file_name_len,"\0",0,info,file->cur);
						CHECK_MEM(ret);
						local->form_flag=HTTP_FORM_FILE;
					}else{
						local->null_file=1;
					}
				}
				ret=http_get_form_lable_value(local->cache_buf,ret_len,
						name_label,name_label_len,name,&name_len,sizeof(file_name));
				if( local->form_flag == HTTP_FORM_ARG){
					arg->cur=http_array_new_arg( info);
					if( arg->cur == NULL){
						return MEM_ERR;
					}

					myret=http_set_arg(name,name_len,NULL,0,info,arg->cur);
					CHECK_MEM(myret);
					arg->cur->arg_flag=0;
					myret=http_append_arg("\0",1,info,arg->cur);
					CHECK_MEM(myret);
					arg->cur->arg_flag=1;
				}
			}
			continue;
		}else if( local->form_status == HTTP_FORM_BODY){
			switch(*mydata){
				case '-':
					local->myform_label_num++;
					if( local->myform_label_num==local->boundary_cnt){
						local->myform_label_state=LABEL_STATE_BEGIN;
					}
					break;
				default:
					if( local->myform_label_state==LABEL_STATE_BEGIN){
						local->myform_label_state=LABEL_STATE_RUN;
					}else {
					}
					if( local->myform_label_state!=LABEL_STATE_RUN){
						local->myform_label_state=LABEL_STATE_NULL;
					}
					local->myform_label_num=0;

					break;
			}
	
			if( local->myform_label_state == LABEL_STATE_RUN){
				ret=http_wait_label_str(buf,sizeof(buf),mydata,1,info,label,label_len);
			}else{
                		ret=NORMAL_ERR;
			}
	


			if( ret ==NORMAL_ERR){
				if( local->form_flag ==HTTP_FORM_FILE){

					if( file->cur != NULL && local->null_file==0){
						http_file_t* item=file->cur;
						item->raw_file_len=item->raw_file_len+1;
						if( item->value.len+1 >HTTP_FILE_MAX_LEN){
							item->complete_flag=1;
						
						}else{

							ret=http_append_file(mydata,1,info,file->cur);
							CHECK_MEM(myret);
						}
					}
				}else{
					ret=http_append_arg(mydata,1,info,arg->cur);
					CHECK_MEM(myret);
				}
			}else{
				if( local->form_flag ==HTTP_FORM_FILE){
				 	if ( file->cur != NULL &&file->cur->value.p!= NULL && file->cur->value.len>0&& local->null_file==0 ){
						file->cur->value.len=file->cur->value.len-label_len-1;
						if( file->cur->value.len<0){
							file->cur->value.len=0;
						}
						file->cur->raw_file_len=file->cur->raw_file_len-label_len-1;
						if( file->cur->raw_file_len <0){
							file->cur->raw_file_len=0;
						}
						file->cur->value.p[file->cur->value.len]='\0';
						file->cur->complete_flag=1;
					}
				}else{
					if ( arg->cur != NULL && arg->cur->value.p!= NULL && arg->cur->value.len>0){
						arg->cur->value.len=arg->cur->value.len-label_len-1;
						if( arg->cur->value.len<0){
							arg->cur->value.len=0;
						}
						arg->cur->value.p[arg->cur->value.len]='\0';
					}
				}
				local->form_status =HTTP_FORM_HEADER;
				local->form_flag=HTTP_FORM_ARG;
			}
			mydata++;
			mylen--;
			continue;
		}

	}
	if( http_get_logic(info,HTTP_LOGIC_STATE) == HTTP_STATE_NONE){
		if( local->form_flag ==HTTP_FORM_FILE ){
		 	if ( file->cur != NULL &&file->cur->value.p!= NULL && file->cur->value.len>0 && local->null_file ==0 ){
				file->cur->value.len=file->cur->value.len-label_len-2;
				if( file->cur->value.len<0){
					file->cur->value.len=0;
				}
				file->cur->raw_file_len=file->cur->raw_file_len-label_len-2;
				if( file->cur->raw_file_len <0){
					file->cur->raw_file_len=0;
				}
				file->cur->value.p[file->cur->value.len]='\0';
				file->cur->complete_flag=1;
			}
		}else{
			if ( arg->cur != NULL && arg->cur->value.p!= NULL && arg->cur->value.len>0){
				arg->cur->value.len=arg->cur->value.len-label_len-2;
				if( arg->cur->value.len<0){
					arg->cur->value.len=0;
				}
				arg->cur->value.p[arg->cur->value.len]='\0';
			}
		}
				
	}
	return 0;
}



int http_parse_body_args(char* data,int data_len,http_info_t* info)
{
	if ( http_get_logic(info,HTTP_IS_MIX_XML) == HTTP_TYPE_XML){
		return 0;
	}
	int ret=0;
	switch (http_get_logic(info,HTTP_LOGIC_CTYPE)){
		case HTTP_TYPE_MULTIFORM:
			ret=http_parse_form(data,data_len,info);
			break;
		case HTTP_TYPE_AMF:
			if ( http_get_int(info,HTTP_INT_CLEN)>http_config.amf_cache_size){
			}
//			http_parse_amf(info,data,data_len);
			break;
		case HTTP_TYPE_FORMENC:
		case HTTP_TYPE_HTML:
		case HTTP_TYPE_PLAIN:
			ret=_http_parse_args(info,data,data_len,0);
			break;
		default:
			ret=_http_parse_args(info,data,data_len,0);
			break;
		

	}
	return ret;
}

#include "http_decode_amf.h"

int http_parse_body(char* data,int data_len,int offset,http_info_t* info)
{
	http_logic_t* logic=http_get_logic_t(info);
	int ret=0;
        if( get_transfer_encoding(info)==0){
                recv_content_len(data,data_len,info);
        }else{
                recv_chunk(data,data_len,info);
        }
	if( logic->dir == 0){
		ret=http_parse_body_args(data,data_len,info);
		CHECK_MEM(ret);
	}
        return data_len;
}



int get_post_boundary(char *begin, char *end, http_info_t *info)
{
	http_set_protocol_str(info,HTTP_STR_BOUNDARY,begin,end-begin);
	http_local_t* local=http_get_local_t(info);
	while( begin<end){
		if( *begin != '-'){
			break;
		}
		local->boundary_cnt++;
		begin++;
	}
	local->boundary_cnt++;
	local->boundary_cnt++;
	return 0;
	
}




int 
http_init_mstate(void) 
{
	http_mstate[HTTP_STATE_INIT].func=http_parse_head;
	http_mstate[HTTP_STATE_HEAD].func=http_parse_head;
	http_mstate[HTTP_STATE_BODY].func=http_parse_body;
	http_mstate[HTTP_STATE_NONE].func=http_parse_head;
	http_mstate[HTTP_STATE_ERROR].func=http_parse_head;

	return 0;
}


int http_table_init()
{
	http_init_mstate();
	http_init_hstate();
	http_init_method_table();
	http_init_header_table();
	http_init_ignore_param();
	return 0;
}


int http_global_init()
{
	apr_initialize();
	http_table_init();
	return 0;
}

int http_global_clean()
{
	apr_terminate();
	return 0;

}
int http_reset_header(http_info_t* info)
{
	http_hctrl_t *header=&info->http->header_1;;
	header->cur=NULL;
	header->array=apr_array_make(info->mp, 4, sizeof(http_hline_t *));
	HTTP_MEM_ERR(info,header->array);
	header->buf.current=0;
	//header->buf.max_len=4095;
	return 0;
}
int http_reset_cookie(http_info_t* info)
{
        http_ckctrl_t *cookie=&info->http->cookie_1;
	cookie->array=apr_array_make(info->mp, 4, sizeof(http_cookie_t *));
	HTTP_MEM_ERR(info,cookie->array);
	return 0;
}
int http_init_array_after_request(http_info_t* info)
{
	int ret=0;
	ret=http_reset_header(info);
	CHECK_MEM(ret);
	if( ret <0){
		return NORMAL_ERR;
	}
	ret=http_reset_cookie(info);
	CHECK_MEM(ret);
	if( ret <0){
		return NORMAL_ERR;
	}
	return 0;
}
int http_init_array(http_info_t* info)
{
	http_init_array_after_request(info);
	return 0;
}
int http_reset_response(http_info_t* info)
{
	HTTP_FUNC_0(info);
	http_init_local_after_request(info);
	http_init_logic_after_request(info);
	http_init_int_after_request(info);
	http_init_str_after_request(info);
	int ret=http_init_array_after_request(info);
	return ret;
}
int http_clean(http_info_t* info)
{
	assert(info);
	if (info->mp){
                apr_pool_destroy(info->mp);
		info->mp=NULL;
		info->http=NULL;
		info->error_line=0;
		info->mem_error=0;
		info->ver_error=0;
	}
	return 0;
	
}



http_protocol_t * 
http_get_protocol_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	return &info->http->protocol;
}

http_hctrl_t * 
http_get_header_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	if( info->http->logic.dir == 0){
		return &info->http->header_0;
	}else{
		return &info->http->header_1;
	}
}

http_fctrl_t * 
http_get_file_t(http_info_t* info)
{
	
	assert(info);
	assert(info->http);
	return &info->http->file;
}

http_avctrl_t * 
http_get_av_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	return &info->http->av;
}

http_ckctrl_t * 
http_get_cookie_t(http_info_t* info)
{
	assert(info);
	assert(info->http);
	if( info->http->logic.dir == 0){
		return &info->http->cookie_0;
	}else{
		return &info->http->cookie_1;
	}
}

int 
http_set_state(http_info_t *info,int status)
{
	return http_set_status(info,status) ;
}

int 
http_clear_new_args(http_info_t *info)
{
	HTTP_FUNC_0(info);
	http_argctrl_t *arg=http_get_arg_t(info);
	assert(arg);
	if( arg->cur != NULL ){
		if (arg->cur->complete_flag==1){
			arg->new_num=0;
		}else{
			arg->new_num=1;
		}
	}else{
			arg->new_num=0;
	}
	return 0;
}

int 
http_get_new_arg_cnt(http_info_t *info)
{
	HTTP_FUNC_0(info);
	http_argctrl_t *arg=http_get_arg_t(info);
	assert(arg);
        int num=arg->new_num;
	if ( arg->cur == NULL){
		return num;
	}
	if( arg->cur->complete_flag==1){
        	return num;
	}else{
		if( num-1>0){
			return num-1;
		}else{
			return 0;
		}
	}

}

http_arg_t * 
http_get_new_arg(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);
        http_argctrl_t* ctl=http_get_arg_t(info);
	assert(ctl);
	assert(ctl->array);
	int real_num=ctl->array->nelts-ctl->new_num+num;

	if( real_num > ctl->array->nelts){
		return NULL;
	}
	http_arg_t* arg=((http_arg_t **)ctl->array->elts)[real_num];
	if( arg->name.p== NULL){
		arg->name.len=0;
		arg->name.p="\0";
	}else if( arg->name.len ==1 && arg->name.p[0]=='\0'){
		arg->name.len=0;
	}
	if( arg->value.p== NULL){
		arg->value.len=0;
		arg->value.p="\0";
	}else if( arg->value.len ==1 && arg->value.p[0]=='\0'){
		arg->value.len=0;
	}
      
        return  arg;
}

int 
http_clear_new_file_flag(http_info_t *info)
{
	HTTP_FUNC_0(info);
	http_fctrl_t *file=http_get_file_t(info);
	assert(file);
	if ( file->cur == NULL){
		file->new_num=0;
		return 0;
	}
	if( file->cur->complete_flag==1){
		file->new_num=0;
	}else{
		file->new_num=1;
	}
	return 0;

}

int 
http_get_new_file_cnt(http_info_t *info)
{
	HTTP_FUNC_0(info);
	http_fctrl_t *file=http_get_file_t(info);
	assert(file);
	if ( file->cur == NULL){
		return file->new_num;
	}
	if( file->cur->complete_flag==1){
        	return file->new_num;
	}else{
		if( file->new_num-1>0){
			return file->new_num-1;
		}else{
			return 0;
		}
	}
}

http_file_t * 
http_get_new_file(http_info_t* info,int num)
{
	HTTP_FUNC_NULL(info);

        http_fctrl_t* file=http_get_file_t(info);
	assert(file);
	assert(file->array);
	int real_num=file->array->nelts-file->new_num+num;
	if( real_num >file->array->nelts ){
		return NULL;
	}

        return  ((http_file_t **)file->array->elts)[real_num];
}



int 
_http_parse_av(char* data, int len, http_info_t* info)
{
	char* mydata=data;
	int mylen=len;
	int ret=0;
	char label[256]={0};
	char *file_name;
	int file_name_len=0;
	int label_len=0;
	char* file_label="filename=\"";
	int file_label_len=strlen("filename=\"");
	char* h_begin=NULL;
	char* b_begin=NULL;
	char* h_end=NULL;
	char* b_end=NULL;

	http_protocol_t* pro = http_get_protocol_t(info);	
	
	assert(pro);
	if( pro->boundary.len <=0){
		return 0;
	}
	snprintf(label,sizeof(label),"--%s",pro->boundary.p);
	label_len=strlen(label);
	
	http_avctrl_t* av = http_get_av_t(info);

	h_begin=strlstr(mydata,mylen,label,label_len);
	if( h_begin == NULL){
		return 0;
	}
	mydata=h_begin+label_len;
	mylen=mylen-(h_begin-mydata);
	
	int end_flag=0;
	while(mylen>0){

		h_end=strlstr(mydata,mylen,"\r\n\r\n",4);
		if( h_end== NULL){
			break;
		}
		mylen=mylen-(h_end-mydata);
		mydata=h_end+4;
		ret=http_get_form_lable_value2(h_begin+label_len,h_end-h_begin-label_len,file_label,file_label_len,&file_name,&file_name_len,sizeof(file_name));
		if( ret == NORMAL_ERR){
			continue;
		}
		
		b_begin=h_end+4;
		b_end=strlstr(mydata,mylen,label,label_len);
		if( b_end == NULL){
			end_flag=1;	
		}else{
			mylen=mylen-(b_end-mydata);
			mydata=b_end+label_len;
		}

		av->cur=http_array_new_av( info);
		if( av->cur == NULL){
			return MEM_ERR;
		}
		
		av->cur->name.p=file_name;
		av->cur->name.len=file_name_len;
		
		av->cur->value.p=b_begin;
		if( end_flag == 0){
			av->cur->value.len=b_end-b_begin-2;
		}else{
			av->cur->value.len=mydata+mylen-b_begin;
			break;
		}
		
		h_begin=b_end+label_len;
	}
	
	return 0;
}

int 
http_parse_av(char* data, int data_len, http_info_t* info)
{

	HTTP_FUNC_0(info);
	if( unlikely(data== NULL)){
		return 0;
	}
	char *tmp_data = data + http_get_int(info,HTTP_INT_HLEN);
        int tmp_data_len = data_len - http_get_int(info,HTTP_INT_HLEN);
	int content_len=http_get_int(info,HTTP_INT_CLEN);
	if( tmp_data_len>content_len){
		printf(" parse av len error\n");
		return -1;
	}

        return _http_parse_av(tmp_data,tmp_data_len,info);

}
int 
http_parse_amf(char* data, int data_len, http_info_t* info)
{

	HTTP_FUNC_0(info);
	if( unlikely(data== NULL)){
		return 0;
	}
	char *tmp_data = data + http_get_int(info,HTTP_INT_HLEN);
        int tmp_data_len = data_len - http_get_int(info,HTTP_INT_HLEN);
	int content_len=http_get_int(info,HTTP_INT_CLEN);
	if( tmp_data_len!=content_len){
		printf(" parse amf len error\n");
		return -1;
	}

        _http_parse_amf(info,tmp_data,tmp_data_len);
	return 0;

}
