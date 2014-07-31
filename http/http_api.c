#include "http.h"
#include "http_basic.h"
#include "http_api.h"
int http_get_logic(http_t *info, int index)
{
	logic_t* logic=http_get_logic_t(info);
	switch( index){
		case HTTP_LOGIC_METHOD:
			return logic->method;
			break;
		case HTTP_LOGIC_IS_CLOSED:
			return logic->is_closed;
			break;
		case HTTP_LOGIC_CONTENT_TYPE:
			return logic->content_type;
			break;
		case HTTP_LOGIC_STATE:
			if( logic->dir == HTTP_LOGIC_FALSE){
				return logic->state_0;
			}else{
				return logic->state_1;
			}
			break;
		
		case HTTP_LOGIC_IS_CHUNKED:
			return logic->is_chunked;
			break;
		default:
			return -1;
	}
	return -1;
}

int 
http_get_int(http_t *info, int index)
{
	http_int_t* http_int=http_get_int_t(info);
	switch( index) {
		case HTTP_INT_RET_CODE:
			return http_int->retcode;
			break;
		case HTTP_INT_HEADER_LEN:
			return http_int->header_len;
			break;
		case HTTP_INT_CONTENT_LEN:
			return http_int->content_len;
			break;
		case HTTP_INT_MAX_HEADER_LINE_LEN:
			return http_int->max_header_line_len;
			break;
		case HTTP_INT_BODY_LEN:
			return http_int->body_len;
			break;
		case HTTP_INT_URL_ARG_LEN:
			return http_int->url_arg_len;
			break;			
		case HTTP_INT_URL_ARG_CNT:
			return http_int->url_arg_cnt;
			break;
		case HTTP_INT_REQUEST_LEN:
			return http_int->header_len+http_int->body_len;
			break;
		case HTTP_INT_HEADER_LINE_NUM:	
			return http_int->header_line_num;
			break;
		case HTTP_INT_COOKIE_NUM:	
			return http_int->header_cookie_num;
			break;
		case HTTP_INT_RANGE_NUM:	
			return http_int->header_range_num;
			break;
		default:
			break;
	}

	return 0;
}

char * _get_http_str(x_str_t str)
{
	if( str.p != NULL){
		return str.p;
	}else{
		return "\0";
	}
}

int 
_get_http_str_len(x_str_t str)
{
	return str.len;
}

char * http_get_str(http_t *info, int index)
{
	http_str_t* str=http_get_str_t(info);
	switch( index){
		case HTTP_STR_HOST:
			return _get_http_str(str->host);
			break;
		case HTTP_STR_COOKIE:
			return _get_http_str(str->cookie);
		case HTTP_STR_URL_AND_ARG:
			return _get_http_str(str->url_and_arg);
			break;
		case HTTP_STR_URL:
			return _get_http_str(str->url);
			break;
		case HTTP_STR_AGENT:
			return _get_http_str(str->agent);
			break;
		case HTTP_STR_AUTH:
			return _get_http_str(str->auth);
			break;
		case HTTP_STR_USER:
			return _get_http_str(str->auth_user);
			break;
		case HTTP_STR_PASSWD:
			return _get_http_str(str->auth_pass);
			break;
		case HTTP_STR_REFER:
			return _get_http_str(str->reference);
			break;
		case HTTP_STR_LOCATION:
			return _get_http_str(str->location);
			break;
		case HTTP_STR_RAW_URL:
			return _get_http_str(str->raw_url);
			break;
		case HTTP_STR_CONTENT_TYPE:
			return _get_http_str(str->content_type);
			break;
		case HTTP_STR_RETCODE:
			return _get_http_str(str->retcode);
			break;
		
		default:
			break;
	}
	return "\0";
}
			
int http_get_str_len(http_t *info, int index)
{
	http_str_t* str=http_get_str_t(info);
	switch( index){
		case HTTP_STR_HOST:
			return _get_http_str_len(str->host);
			break;
		case HTTP_STR_COOKIE:
			return _get_http_str_len(str->cookie);
			break;
		case HTTP_STR_URL_AND_ARG:
			return _get_http_str_len(str->url_and_arg);
			break;
		case HTTP_STR_URL:
			return _get_http_str_len(str->url);
			break;
		case HTTP_STR_AGENT:
			return _get_http_str_len(str->agent);
			break;
		case HTTP_STR_AUTH:
			return _get_http_str_len(str->auth);
			break;
		case HTTP_STR_USER:
			return _get_http_str_len(str->auth_user);
			break;
		case HTTP_STR_PASSWD:
			return _get_http_str_len(str->auth_pass);
			break;
		case HTTP_STR_REFER:
			return _get_http_str_len(str->reference);
			break;
		case HTTP_STR_LOCATION:
			return _get_http_str_len(str->location);
			break;
		case HTTP_STR_RAW_URL:
			return _get_http_str_len(str->raw_url);
			break;
		case HTTP_STR_CONTENT_TYPE:
			return _get_http_str_len(str->content_type);
			break;
		case HTTP_STR_RETCODE:
			return _get_http_str_len(str->retcode);
			break;
		
		default:
			break;
	}
	return 0;
}
			

