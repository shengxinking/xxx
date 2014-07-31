#include "http.h"
#include "http_table.h"
#include "http_basic.h"
#include "http_api.h"
#include "http_header.h"

//#define debug_http
#ifdef debug_http
char sbuf[40960];
int sbuf_len;
char* http_get_data(void* s,int* len)
{
        *len=sbuf_len;
        return &sbuf[0];
}
#endif
char* jump_buf="\r\n\r\n";
int jump_buf_len=4;
int http_global_init()
{
	http_init_mstate();
	http_init_header_table();
	http_init_method_table();
	http_init_header_state();

	return 0;
}
int http_init(http_t* info,void* ssn)
{
//	memset(info,0x00,sizeof(http_t));
	info->mp=NULL;
        int ret=0;
        ret = apr_pool_create(&info->mp, NULL);
        if (ret != APR_SUCCESS) {
		return 0;
        }

        apr_allocator_t *pa = apr_pool_allocator_get(info->mp);
        if (pa) {
                apr_allocator_max_free_set(pa, 1);
        }

	info->p_cnt=0;	
	info->arg.max_num=32;
	local_t* local=http_get_local_t(info);

	local->form_status=HTTP_FORM_BEGIN;
	local->form_flag=HTTP_FORM_ARG;
	http_int_t* http_int=http_get_int_t(info);
	memset(http_int,0x00,sizeof(http_int_t));	
	http_arg_ctrl_t* arg=http_get_arg_t(info);
	memset(arg,0x00,sizeof(http_arg_ctrl_t));
	memset(&info->str,0x00,sizeof(info->str));
	info->arg.max_num=32;
	info->init_flag=0;
	return 0;
}
int http_clean(http_t* info)
{
        if (info->mp){
                apr_pool_destroy(info->mp);
                info->mp=NULL;
        }
        return 0;

}


int http_reset_response(http_t* info)
{
	return 0;
}
int http_check_before_parse(char* data,int data_len,http_t* info,int dir)
{
	int ret=0;
	if( info->init_flag == 0){
                ret=http_init(info,NULL);
		if( ret == -1){
			return -1;
		}
	}
	logic_t* logic=http_get_logic_t(info);
        http_str_t* str=http_get_str_t(info);
        http_int_t* http_int=http_get_int_t(info);
        local_t* local=http_get_local_t(info);
	
	int state=http_get_logic(info,HTTP_LOGIC_STATE);
	int code=http_int->retcode;
	int ldir=logic->dir;
	if ( dir == 0){
        	if( ldir == 1 ){
               		if( code != 100){
                       		ret=http_init(info,info->ssn);
				if( ret == -1){
					return -1;
				}
                	}

		#if debug_http_
			sbuf_len=0;
		#endif
        	}
		
        	if( state == HTTP_STATE_NONE && code != 100){
                       	ret=http_init(info,info->ssn);
			if( ret == -1){
				return -1;
			}
        	}
        	if ( code == 100){
               		//http_int->header_len=0;
        	}
	}
	if( dir == 1){
        	if( ldir == 0 ){
               		if( code == 100){
				http_reset_response(info);
			}
		}
	}

	logic=http_get_logic_t(info);
        str=http_get_str_t(info);
        http_int=http_get_int_t(info);
        local=http_get_local_t(info);
	if ( dir == 1){
        	if ( logic->dir == 0 ){
			local->sub_status=HTTP_HEADER_STATE_CHAR0;
			local->cache_buf_len=0;
		#ifdef debug_http
			sbuf_len=0;
		#endif
			local->last_len=0;
		}
        }
	logic->dir=dir;

	return 0;
}
int http_check_after_parse(char* data,int data_len,http_t* info,int dir)
{
        logic_t* logic=http_get_logic_t(info);
        if ( dir == 1){
                if ( http_get_logic(info,HTTP_LOGIC_STATE)== HTTP_STATE_BODY && logic->method == HTTP_METHOD_HEAD){
                        logic->state_1  =        HTTP_STATE_NONE;
                }
        }
        return 0;
}
/* define the header end  */
int s_http_parse(char* data,int data_len,http_t* http)
{
        local_t* local=http_get_local_t(http);
        int ret=-1;
        char* begin=data;
        int mylen=data_len;
        int lstate=local->state;
/*jump_buf:"[0]\r [1]\n [2]\r[3]\n */
        while(mylen>0){
                if( jump_buf[lstate] == *begin){
                        lstate++;
                }else{
                         lstate=0;
                }
                if( jump_buf[lstate]==*begin){
                        lstate++;
                }
                if( lstate==jump_buf_len){
                    ret=0;
                	begin++;
                	mylen--;
					break;
                }else{
                }

                local->state=lstate;

                begin++;
                mylen--;
	}
	local->state=lstate;
#ifdef debug_http
        memcpy(&sbuf[0]+sbuf_len,data,data_len-mylen);
        sbuf_len=sbuf_len+data_len-mylen;
#endif
	//http_int->header_len+=data_len-mylen;
	//printf("  header len  %d %d %d\n",http_int->header_len,data_len,mylen);
        return data_len-mylen;
}
int http_parse_head(char* data,int data_len,int offset,http_t* info,int dir)
{
	int ret=0;

	int st;

	int flag=0;
	char* begin=NULL;
	char* split=NULL;
	char* end=NULL;

	http_init(info,NULL);
	int num=0;
	logic_t* logic=http_get_logic_t(info);
	logic->dir=dir;
	int tmp_flag=0;
	
	http_int_t* http_int=http_get_int_t(info);
	http_int->header_len+=data_len;
	
	st=HTTP_HEADER_STATE_CHAR0;
	while( data_len >0){
		switch (*data){
			case '\r': 
				st=http_header_state[st][1].state; 
				break; 
			case '\n': 
				st=http_header_state[st][2].state; 
				break; 
			case '%': 
				SET_NEED_URL_DECODE(flag); 
				//flag =1;
				st=http_header_state[st][0].state; 
				break; 
			case '&': 
				SET_NEED_HTML_DECODE(flag); 
				st=http_header_state[st][0].state; 
				break; 
			case '\\': 
				SET_NEED_ESCAPE_DECODE(flag); 
				st=http_header_state[st][0].state; 
				break; 
			case ':':
				if( tmp_flag ==0){
					split=data;
					tmp_flag=1;
				}
			default: 
				st=http_header_state[st][0].state; 
				break; 
		}
		
		switch( st){
			case HTTP_HEADER_STATE_CHAR1:
				begin=data;
				break;
			case HTTP_HEADER_STATE_N1:
				end=data-1;
				st=HTTP_HEADER_STATE_CHAR0;
				info->tmp_flag=flag;
				tmp_flag=0;
				flag=0;
				http_parse_line(info,begin,split,end,dir,num++);
				info->http_int.header_line_num++;

				break;
			default:
				break;

		}
		data++;
		data_len--;
	}
	http_int->header_len+=data_len;
	
	http_set_state(info,HTTP_STATE_HEAD)	;
	return ret;
}
/*int http_parse_body(char* data,int data_len,int offset,http_t* info)
{
	local_t* local=http_get_local_t(info);
	local->un_recv_len=local->un_recv_len-data_len;
	return data_len;
	
}*/
int http_parse(char* data,int data_len,http_t* info,int dir)
{
        int ret=0;
        int offset=0;
        //int raw_len=data_len;
        volatile int status;

        ret=http_check_before_parse(data,data_len,info,dir);
        if( ret == -1){
                goto out;
        }
        //local_t* local=http_get_local_t(info);

        while( data_len>0){

                status=http_get_logic(info,HTTP_LOGIC_STATE);

                if ( status == HTTP_STATE_NONE && data_len>0){
                        goto out;
                }

                ret=http_state[status].func(data+offset,data_len,offset,info);
                if( ret == -1){
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
        return ret;
}

