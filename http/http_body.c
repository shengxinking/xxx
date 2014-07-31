#include "http.h"
#include "http_basic.h"
#include "http_api.h"
int http_read_line(char* data,int data_len,http_t* info)
{
	int len=data_len;
	local_t* local=http_get_local_t(info);
	if((unsigned int) (local->cache_buf_len+data_len )> sizeof( local->cache_buf)){
		return -1;
	}
	memcpy( local->cache_buf+local->cache_buf_len,data,data_len);
	local->cache_buf_len=local->cache_buf_len+len;
	local->cache_buf[local->cache_buf_len]='\0';
	char *p=strlstr(local->cache_buf,local->cache_buf_len,"\r\n",2);
	if( p == NULL){
		return -1;
	}
	local->cache_buf_len=0;
	return 0;
	
	
}


int recv_chunk(char* data,int data_len,http_t* info)
{
	char *chunked_point=NULL;
	int chunked_left=0;
	int read_len=0;	

	local_t* local=http_get_local_t(info);

	chunked_point=data;
	chunked_left=data_len;

	while(chunked_left>0){ 
		if( local->un_recv_chunked_len>0){
			if( chunked_left<local->un_recv_chunked_len){
				local->un_recv_chunked_len=local->un_recv_chunked_len-chunked_left;
				break;
			}else {
				if( local->chunk_flag==HTTP_CHUNK_FIN){
					local->un_recv_chunked_len=0;
					break;
				}
				
				chunked_point=chunked_point+local->un_recv_chunked_len;
				chunked_left=chunked_left-local->un_recv_chunked_len;
				local->un_recv_chunked_len=0;
				local->chunk_flag=HTTP_CHUNK_SIZE;
				continue;
			}
		}
		read_len=http_read_line(chunked_point,1,info);
		chunked_point=chunked_point+1;
		chunked_left=chunked_left-1;

		if(read_len==-1){
			continue; 
		}
		local->un_recv_chunked_len=strtol(local->cache_buf, NULL, 16);
		if ( local->un_recv_chunked_len <0 ){
			break; 
		}
		if (local->un_recv_chunked_len == 0){
			local->un_recv_chunked_len=2;
			local->chunk_flag=HTTP_CHUNK_FIN;
			continue;
					
		}
		local->un_recv_chunked_len=local->un_recv_chunked_len+2;
		continue;
		
	}
	if ( local->chunk_flag==HTTP_CHUNK_FIN && local->un_recv_chunked_len == 0){
		http_set_state(info,HTTP_STATE_NONE);
	}else{
			
	}
	return 0;

		
}

int http_parse_body_args(char* data,int data_len,http_t* info)
{
        int ret=0;
	if( data_len <=0){
		return 0;
	}
        switch (http_get_logic(info,HTTP_LOGIC_CONTENT_TYPE)){
                case HTTP_TYPE_MULTIFORM:
                        ret=http_parse_form(data,data_len,info);
                        break;
                case HTTP_TYPE_AMF:
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

int http_parse_body(char* data,int len,http_t* info)
{
	DBGH("body len =%d\n",len);
	http_int_t* http_int=http_get_int_t(info);
	local_t* local=http_get_local_t(info);
	logic_t* logic=http_get_logic_t(info);

	if (len <= 0) {
		return 0;
	}
	http_int->body_len=http_int->body_len+len;
        if ( logic->is_chunked==HTTP_LOGIC_TRUE){
		recv_chunk(data,len,info);
	}else{

		local->un_recv_content_len=local->un_recv_content_len-len;
		if( local->un_recv_content_len <=0){
			http_set_state(info,HTTP_STATE_NONE);	
		}else{
			http_set_state(info,HTTP_STATE_BODY);	
		}
	}
	if ( logic->dir==0){
		http_parse_body_args(data,len,info);
	}
	return 0;
}
