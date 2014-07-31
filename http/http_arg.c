#include "http.h"
#include "http_api.h"
#include "http_basic.h"
int http_get_arg_cnt(http_t* info)
{
	http_arg_ctrl_t* ctl=http_get_arg_t(info);
	return ctl->num;
}
http_arg_t*  http_get_arg(http_t* info,int num)
{
	http_arg_ctrl_t* ctl=http_get_arg_t(info);
	if(num> ctl->num){
		return NULL;
	}
	return &ctl->arg[num];

}
http_arg_t* http_get_empty_arg(http_t* info)
{
	http_arg_ctrl_t* ctl=http_get_arg_t(info);
	http_arg_t* arg=NULL;
	if( ctl->num>= ctl->max_num){
		return NULL;
	}
	arg=&ctl->arg[ctl->num++];
	return arg;
}
int http_decode_arg(http_arg_t* item,http_t* info)
{
        unsigned char buf[8192];
        int ret=0;
        if( GET_NEED_DECODE(item->decode_flag) != 0 ){
                ret=http_decode_string(buf,sizeof(buf),(unsigned char*)item->name.p,item->name.len,item->decode_flag);
                if( ret>0 && ret<item->name.len){
                        memcpy(item->name.p,buf,ret);
                        item->name.len=ret;
                        item->name.p[item->name.len]='\0';
                }
                ret=http_decode_string(buf,sizeof(buf),(unsigned char*)item->value.p,item->value.len,item->decode_flag);
                if( ret>0 && ret< item->value.len){
                        memcpy(item->value.p,buf,ret);
                        item->value.len=ret;
                        item->value.p[item->value.len]='\0';
                }
        } else {
	}
        return 0;
}

int http_append_arg(char* value,int value_len,http_t* info,http_arg_t* item)
{
	http_arg_ctrl_t *arg=http_get_arg_t(info);
	local_t *local=http_get_local_t(info);


	if( value == NULL || value_len ==0){
		return 0;
	}
	int ret=0;
	if( item->arg_flag ==0){
		ret=http_append_str(info,&item->name,value,value_len);

	}else{
		ret=http_append_str(info,&item->value,value,value_len);
	}
	if(1|| local->force_decode == 1 || (value[0] == '\0' && item->arg_flag ==1)|| http_get_logic(info,HTTP_LOGIC_STATE)==HTTP_STATE_NONE){
	http_decode_arg(arg->cur,info);		
	}
	return 0;
}


int _http_parse_args(http_t* info,char* data,int data_len,int in_url)
{
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

	http_arg_ctrl_t* arg=http_get_arg_t(info);

	if( arg->cur == NULL){
		arg->cur=http_get_empty_arg( info);
		if( arg->cur == NULL){
			return -1;
		}
	}
	myflag=&arg->cur->decode_flag;
	while(len>0){
		switch(*begin){
			case '%':
            case '+':
                SET_NEED_URL_DECODE(*myflag);
				DBGH("set url decode \n");
				if( tmp_data== NULL){
					tmp_data=begin;
				}
				tmp_len++;
				if( in_url ){
					http_int->url_arg_len=http_int->url_arg_len+1;
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
					http_int->url_arg_len=http_int->url_arg_len+1;
				}
				break;
            case '&':
				ret=http_append_arg(tmp_data,tmp_len,info,arg->cur);	
				tmp_data=NULL;
				tmp_len=0;
				if( in_url){
					http_int->url_arg_cnt++;
				}
				arg->cur->complete_flag=1;
				arg->cur=http_get_empty_arg( info);
				if( arg->cur == NULL){
					return -1;
				}
				name_len=0;
				value_len=0;
				myflag=&arg->cur->decode_flag;
				mylen=&name_len;
				arg->cur->arg_flag=0;
                                break;
			case '=':
				if( arg->cur->arg_flag != 1){
					ret=http_append_arg(tmp_data,tmp_len,info,arg->cur);	
					tmp_data=NULL;
					tmp_len=0;
					ret=http_append_arg("\0",1,info,arg->cur);	
					myflag=&arg->cur->decode_flag;
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
				if( in_url ){
					http_int->url_arg_len=http_int->url_arg_len+1;
				}
				break;
		}
		begin++;
		len--;
		
	} 	
	
	ret=http_append_arg(tmp_data,tmp_len,info,arg->cur);	
	if( in_url == 0&& http_get_logic(info,HTTP_LOGIC_STATE)==HTTP_STATE_NONE){
		ret=http_append_arg("\0",1,info,arg->cur);	
		arg->cur->complete_flag=1;
	}
	if( in_url){
		ret=http_append_arg("\0",1,info,arg->cur);	
		http_int->url_arg_cnt++;
		arg->cur->complete_flag=1;
	}
	
	return 0;
}
