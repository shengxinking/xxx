#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http_modify.h"
#include "http.h"
#include "http_basic.h"
#include "http_api.h"

#define MAX_LINE_LEN 2000
char *x_memrstr(const char *haystack, int range, const char *needle, int needle_len)
{
        char *p = (char *) haystack + range;

        if((needle_len < 1 || !needle))
                return (char *)haystack;

        if((needle_len > range || !range || !haystack))
                return NULL;

        p -= needle_len;

        while (p >= haystack) {
                if (!memcmp(p, needle, needle_len)) {
                        return p;
                }
                p--;
        }

        return NULL;
}

char *x_memcasestr(const char *haystack, int range, const char *needle, int needle_len)
{
        char *p = (char *) haystack;
        int h_len = range;

        if((needle_len < 1 || !needle))
                return (char *)haystack;

        if((needle_len > range || !range || !haystack))
                return NULL;


        while(h_len >= needle_len) {
                if(!strncasecmp(p, needle, needle_len))
                        return p;
                p++;
                h_len--;
        }

        return NULL;
}

static int 
_http_get_header_end_line( char* data,int data_len,char** begin,char** end)
{
	if( data == NULL || data_len<=0 || begin == NULL || end== NULL){
		return -1;
	}	
	*begin=x_memcasestr(data,data_len,"\r\n\r\n",4);
	if( *begin == NULL ){
		return -1;
	}
	*begin=*begin+2;
	*end=*begin;
	return 0;
}

static int 
_http_locate_buf(char* data,int data_len,char* front,int front_len,char* back,int back_len,char** begin,char** end)
{
	if( data == NULL || front == NULL || back == NULL || begin ==NULL || end == NULL){
		return -1;
	}
	if( data_len<=0 || front_len <=0 || back_len <=0 ){
		return -1;
	}
	*begin =x_memcasestr(data,data_len,front,front_len);
	if( *begin == NULL){
		return -1;
	} 
	*end=x_memcasestr(*begin,data+data_len-*begin,back,back_len);
	if( *end== NULL){
		return -1;
	}
	return 0;
}

static int 
_http_modify_stream( char* new_data, int max_new_data_len,char* data,int data_len, char* begin,char* end,char* new,int new_len)
{
	//begin == end is valid  && new_len == 0 is valid too

	if ( new_data == NULL || data == NULL || data_len <=0 || begin == NULL || end == NULL 
		|| end<begin ||new_len <0){
		return -1;
	}
	
	
	if( data_len -(end-begin) +new_len >max_new_data_len){
		return -1;
	}
	int new_data_len=0;

	memmove(new_data,data,begin-data);
	new_data_len=begin-data;

	memmove(new_data+new_data_len+new_len,
			end,data+data_len-end);

	if( new_len !=0){
		memmove(new_data+new_data_len,new,new_len);
	}
	new_data_len=data_len -(end-begin) +new_len;
	
	return new_data_len;
}

static int 
_http_locate_line_value(char* data,int data_len,char* label,int label_len,char** begin,char** end)
{
	if( data== NULL || data_len<=0 || label == NULL || label_len<=0||begin==NULL || end==NULL){
		return -1;
	}

	char *start=data;
	int flag=0;
	int cnt=0;
	while (  start< data+data_len && cnt <128){
		flag=0;
        	*begin = x_memcasestr(start, data+data_len-start, label, label_len);
		if( *begin == NULL){
			return -1;
		}
		int jump_len=*begin-data;
		char *last_end=x_memrstr(data,jump_len,"\r\n",2);
		if( last_end == NULL){
			return -1;
		}
		last_end++;
		last_end++;
		int cnt2=0;
		while( last_end<*begin && cnt2<128){
			if( *last_end != ' ' || *last_end != '\t'){
				start=*begin+label_len;
				flag=1;
				break;
			}
	
			last_end++;
			cnt2++;
		}
		if( flag==1){
			cnt++;
			continue;
		}
		*begin=*begin+label_len;
	
      		*end=x_memcasestr(*begin, data_len-(*begin-data), "\r", 1);
		if( *end == NULL){
			return -1;
		}
		return 0;
	}
	return -1;
}


static int 
_http_insert_line(http_modify_stream_t* stream,char* label,int label_len,char* value,int value_len)
{

	char* begin=NULL; 
	char* end=NULL; 
	if( _http_get_header_end_line( stream->data,stream->data_len,&begin,&end)<0){ 
		return -1; 
	} 
	char buf[8192]={0}; 
	if( (unsigned int)(value_len+label_len+2)>sizeof(buf)){
		return -1;
	}
	snprintf(buf,sizeof(buf),"%s%s\r\n",label,value); 
	int new_len = _http_modify_stream(stream->new_data, stream->max_new_data_len,
			stream->data, stream->data_len, begin,end,buf,strlen(buf)); 
	if ( new_len >0 ){ 
		stream->data=stream->new_data; 
		stream->data_len=new_len; 
		stream->change_flag=0; 
		return stream->data_len; 
	}else{
	       	return -1;
       	}

}
static int 
_http_update_line(http_modify_stream_t* stream,char* label,int label_len,char* value,int value_len)
{

	char* begin=NULL; 
	char* end=NULL; 
	if( _http_locate_line_value( stream->data,stream->data_len,label,label_len,&begin,&end)<0){ 
		return -1; 
	} 
	char buf[8192]={0}; 
	int new_len=0;
	if( (unsigned int)value_len >sizeof( buf)){
		return -1;
	}

	snprintf(buf,sizeof(buf),"%s",value); 
	if( value_len == 0){
		 new_len = _http_modify_stream(stream->new_data, stream->max_new_data_len,
                       stream->data, stream->data_len, begin-label_len,end+2,buf,strlen(buf));

	}else{
		new_len = _http_modify_stream(stream->new_data, stream->max_new_data_len,
			stream->data, stream->data_len, begin,end,buf,strlen(buf)); 
	}
	if ( new_len >0 ){ 
		stream->data=stream->new_data; 
		stream->data_len=new_len; 
		stream->change_flag=0; 
		return stream->data_len; 
	}else{
	       	return -1;
       	}

}

static int 
_http_append_line(http_modify_stream_t* stream,char* label,int label_len,char* value,int value_len)
{

	char* begin=NULL; 
	char* end=NULL; 
	char slabel[256]={0};
	int slabel_len=0;
	snprintf(slabel,sizeof(slabel),"\n%s",label);
	slabel_len=strlen(slabel);
	
	if( _http_locate_buf( stream->data,stream->data_len,slabel,slabel_len,
				                "\r",1,&begin,&end) <0){
	       	return -1;
       	}

	char buf[2048]={0}; 
	if( (unsigned int)value_len >sizeof( buf)){
		return -1;
	}
	snprintf(buf,sizeof(buf),"%s",value); 
	int new_len = _http_modify_stream(stream->new_data, stream->max_new_data_len,
			stream->data, stream->data_len, end,end,buf,strlen(buf)); 

	if ( new_len >0 ){ 
		stream->data=stream->new_data; 
		stream->data_len=new_len; 
		stream->change_flag=0; 
		return stream->data_len; 
	}else{
	       	return -1;
       	}

}

static int 
_http_remove_line(http_modify_stream_t* stream,char* line,int len)
{
	char* begin=NULL;
        char* end=NULL;
        if( _http_locate_line_value( stream->data,stream->data_len,line,
				len,&begin,&end)<0){
		return stream->data_len;
	}
        int new_len = _http_modify_stream(stream->new_data, stream->max_new_data_len,stream->data,
                stream->data_len, begin-len,end+2,"",0);
	if ( new_len >0 ){
			stream->data=stream->new_data;
			stream->data_len=new_len;
			stream->change_flag=0;
			return stream->data_len;
	}else{
			return -1;
	}


}



static int _http_strlen_max(const char * str,int max) 
{
	size_t length = 0 ; 
	if( str == NULL){
		return 0;
	}
	while (*str++ ){
		 ++ length;
		if( length>=(unsigned int)max){
			 return 0; 
		} 
	}
     return  length;
}


static int 
_http_append_forward(http_modify_stream_t* stream,http_t* info,char* ip)
{
	char* slabel="\r\nX-Forwarded-For:";
	int slabel_len=strlen(slabel);
	char* label="X-Forwarded-For:";
	int label_len=strlen(label);
	char buf[2048];
	if( _http_strlen_max(ip,MAX_LINE_LEN) == 0){
		return -1;
	}
	if( NULL == x_memcasestr(stream->data,stream->data_len,slabel,slabel_len)){
		sprintf(buf,"%s",ip);
		return _http_insert_line(stream,label,label_len,ip,strlen(ip));
	}else{
			
		sprintf(buf,",%s",ip);
		return _http_append_line(stream,label,label_len,buf,strlen(buf));
	}

}

static int 
_http_append_gzip(http_modify_stream_t* stream,http_t* info)
{
	char* slabel="\r\nContent-Encoding:";
	int slabel_len=strlen(slabel);

	char* label="Content-Encoding:";
	int label_len=strlen(label);
	if( NULL == x_memcasestr(stream->data,stream->data_len,slabel,slabel_len)){
		return _http_insert_line(stream,label,label_len," gzip",strlen(" gzip"));
	}else{
		return _http_append_line(stream,label,label_len,";gzip",strlen(";gzip"));
	}

}

static int 
_http_replace_content_len(http_modify_stream_t* stream,http_t* info,char* len)
{
	char* label="content-length:";
	int label_len=strlen(label);
	int *content_length_isset;
	local_t *local = http_get_local_t(info);
	logic_t *logic = http_get_logic_t(info);
	if(local == NULL || logic == NULL) {
		return -1;
	}
	if(logic->dir == 0) {
		content_length_isset = &(local->clen_isset_0);
	} else {
		content_length_isset = &(local->clen_isset_1);
	}
	if( *content_length_isset != HTTP_LOGIC_TRUE){
		*content_length_isset = HTTP_LOGIC_TRUE;
		return _http_insert_line(stream,label,label_len,len,strlen(len));
	}else{
		return _http_update_line(stream,label,label_len,len,strlen(len));
	}

}

static int 
_http_replace_host(http_modify_stream_t* stream,http_t* info,char* host)
{
	char* label="Host:";
	int label_len=strlen(label);
	if( http_get_str_len(info,HTTP_STR_HOST) == 0){
		return _http_insert_line(stream,label,label_len,host,strlen(host));
	}else{
		return _http_update_line(stream,label,label_len,host,strlen(host));
	}


}

static int 
_http_update_host(http_modify_stream_t* stream,http_t* info,char* host)
{
	char* label="Host:";
	int label_len=strlen(label);
	if( http_get_str_len(info,HTTP_STR_HOST) == 0){
		return -1;
	}else{
		return _http_update_line(stream,label,label_len,host,strlen(host));
	}
}

static int 
_http_replace_refer(http_modify_stream_t* stream,http_t* info,char* refer)
{
	char* label="Referer:";
	int label_len=strlen(label);
	if( http_get_str_len(info,HTTP_STR_REFER) == 0){
		return _http_insert_line(stream,label,label_len,refer,strlen(refer));
	}else{
		return _http_update_line(stream,label,label_len,refer,strlen(refer));
	}
}



static int 
_http_update_refer(http_modify_stream_t* stream,http_t* info,char* host)
{
	char* label="Referer:";
	int label_len=strlen(label);
	if( http_get_str_len(info,HTTP_STR_REFER) == 0){
		return -1;
	}else{
		return _http_update_line(stream,label,label_len,host,strlen(host));
	}
}

static int 
_http_replace_location(http_modify_stream_t* stream,http_t* info,char* location)
{
	char* label="Location:";
	int label_len=strlen(label);
	if( http_get_str_len(info,HTTP_STR_LOCATION) == 0){
		return _http_insert_line(stream,label,label_len,location,strlen(location));
	}else{
		return _http_update_line(stream,label,label_len,location,strlen(location));
	}

}

static int 
_http_update_location(http_modify_stream_t* stream,http_t* info,char* location)
{
	char* label="Location:";
	int label_len=strlen(label);
	if( http_get_str_len(info,HTTP_STR_LOCATION) == 0){
		return -1;
	}else{
		return _http_update_line(stream,label,label_len,location,strlen(location));
	}

}

static int _http_check_url(http_t* info,char *data, int data_len,int *begin,int *end)
{
	char *line_b = NULL;
	char *line_e = NULL;
	char *label = "\r\n";

	if(data == NULL || data_len <=0 ) {
		return -1;
	}

	
	line_b = data ;
	line_e = x_memcasestr(data,data_len,"\r\n",2);

	if(x_memcasestr(line_e,2,label,strlen(label)) == NULL) {
		//printf("ERR: not get url in pkt\n");
		return -1;
	}

	/*trunk blank at the beginning and ending*/
	while(line_b < line_e) {
		if(*line_b != ' ') {
			break;
		}
		line_b++;
	}
	while(line_b < line_e) {
		if(*line_e != ' ') {
			break;
		}
		line_e--;
	}

	if(line_b >= line_e) {
		return -1;
	}

	//if( x_memcasestr(end-10,9,"HTTP/1.1",8))) {
	//}

	/*GET /test.html HTTP/1.1*/
	/*get the first and end blank*/
	while(line_b < line_e) {
		if(*line_b == ' ') {
			line_b++;
			break;
		}
		line_b++;
	}
	while(line_b < line_e) {
		if(*line_e == ' ') {
			//line_e--;
			break;
		}
		line_e--;
	}

	if(line_b >= line_e) {
		return -1;
	}

	*begin = line_b - data;
	*end = line_e - data;
	return 0;
}

static int 
_http_update_url(http_modify_stream_t* stream,http_t* http_info,char* url)
{
	int begin, end;

	if(_http_check_url(http_info,stream->data,stream->data_len,&begin,&end)) {
		//printf("ERR: url not found\n");
		return -1;
	}
	if(begin <=0 || end <=0) {
		return -1;
	}

       	int new_len=_http_modify_stream(stream->new_data, stream->max_new_data_len,stream->data,
	                stream->data_len, stream->new_data+begin,
			stream->new_data+end,
			url,strlen(url));

	if ( new_len >0 ){ 
		stream->data=stream->new_data; 
		stream->data_len=new_len; 
		stream->change_flag=0; 
		return stream->data_len; 
	}else{ 
		return -1; 
	}

}

static int 
_http_remove_chunk(http_modify_stream_t* stream,http_t* info)
{
	 if( http_get_logic(info,HTTP_LOGIC_IS_CHUNKED) != HTTP_LOGIC_TRUE){
		 return -1;
	 }
	 return _http_remove_line(stream,"transfer-encoding:",strlen("transfer-encoding:"));
}

static int 
_http_remove_gzip(http_modify_stream_t* stream,http_t* info)
{
	return _http_remove_line(stream,"Content-Encoding:",strlen("Content-Encoding:"));
}

static int 
_http_remove_cookie(http_modify_stream_t* stream,http_t* info)
{
	return _http_remove_line(stream,"cookie:",strlen("cookie:"));
}

static int 
_http_remove_auth(http_modify_stream_t* stream,http_t* info)
{
	return _http_remove_line(stream,"Authorization:",strlen("Authorization:"));
}




int http_replace_host(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char host[2048]={0};
	if( _http_strlen_max(pak->host,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(host,sizeof(host)," %s",pak->host);
	
	return _http_replace_host(stream,info,host);
}

int http_update_host(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char buf[2048]={0};
	if( _http_strlen_max(pak->host,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(buf,sizeof(buf)," %s",pak->host);
	return _http_update_host(stream,info,buf);
}
int http_append_xff(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char buf[2048]={0};
	if( _http_strlen_max(pak->xff,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(buf,sizeof(buf)," %s",pak->xff);
	return _http_append_forward(stream,info,buf);
}
int http_append_gzip(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	return _http_append_gzip(stream,info);
}
int http_replace_content_len(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char len[2048]={0};
	if( _http_strlen_max(pak->content_len,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(len,sizeof(len)," %s",pak->content_len);
	return _http_replace_content_len(stream,info,len);
}
int http_replace_refer(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char buf[2048]={0};
	if( _http_strlen_max(pak->refer,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(buf,sizeof(buf)," %s",pak->refer);
	return _http_replace_refer(stream,info,buf);
}
int http_update_refer(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char buf[2048]={0};
	if( _http_strlen_max(pak->refer,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(buf,sizeof(buf)," %s",pak->refer);
	return _http_update_refer(stream,info,buf);
}
int http_replace_location(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char buf[2048]={0};
	if( _http_strlen_max(pak->location,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(buf,sizeof(buf)," %s",pak->location);
	return _http_replace_location(stream,info,buf);
}
int http_update_location(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char buf[2048]={0};
	if( _http_strlen_max(pak->location,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(buf,sizeof(buf)," %s",pak->location);
	return _http_update_location(stream,info,buf);
}
int http_update_url(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	return _http_update_url(stream,info,pak->url);
}
int http_remove_chunk(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	return _http_remove_chunk(stream,info);
}
int http_remove_gzip(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	return _http_remove_gzip(stream,info);
}
int http_remove_cookie(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	return _http_remove_cookie(stream,info);
}
int http_remove_auth(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	return _http_remove_auth(stream,info);
}

static int 
_http_replace_auth(http_modify_stream_t* stream,http_t* info,char* auth)
{
	char* label="Authorization:";
	int label_len=strlen(label);
	if( http_get_str_len(info,HTTP_STR_AUTH) == 0){
		return _http_insert_line(stream,label,label_len,auth,strlen(auth));
	}else{
		return _http_update_line(stream,label,label_len,auth,strlen(auth));
	}


}
int http_replace_auth(http_header_modify_t* pak,http_modify_stream_t* stream,http_t* info)
{
	char auth[2048]={0};
	if( _http_strlen_max(pak->auth,MAX_LINE_LEN) == 0){
		return -1;
	}	
	snprintf(auth,sizeof(auth)," %s",pak->auth);
	
	return _http_replace_auth(stream,info,auth);
}


int http_change_stream(char* new_data,int max_new_data_len,char *data, int data_len,http_t* info,http_header_modify_t* modify)
{
	logic_t* logic = NULL;
	local_t* local = NULL;
	if ( info == NULL){
		goto out;		
	}

	logic = http_get_logic_t(info);
	local = http_get_local_t(info);
	if(logic == NULL || local == NULL) {
		goto out;
	}

	if( new_data == NULL || data == NULL || data_len <=0){
		goto out;
	}
	int head_len=0;
        //head_len=info->http_int.head_len;
	head_len = http_get_int(info,HTTP_INT_HEADER_LEN);
        if ( head_len >data_len){
                head_len=data_len;
        }

	
	http_modify_stream_t stream;
	memset(&stream,0x00,sizeof(http_modify_stream_t));

	stream.new_data=new_data;
	stream.max_new_data_len=max_new_data_len;
	stream.data=data;
	stream.data_len=head_len;
	stream.change_flag=-1;


	if( max_new_data_len<stream.data_len){
		goto out;
	}
	memcpy(stream.new_data,stream.data,stream.data_len);
	stream.data=stream.new_data; 
	stream.data_len=head_len;
	stream.change_flag=0; 

	if( logic->dir ==HTTP_LOGIC_FALSE){
		//ret = _http_remove_cookie_fortiweb(&stream,info);
	}
		
	if( modify->url != NULL && modify->url_len>0){
		_http_update_url(&stream,info,modify->url);
	}
	if( modify->host != NULL && modify->host_len>0){
		_http_replace_host(&stream,info,modify->host);
	}

	if ( stream.change_flag == 0){
		if( max_new_data_len-stream.data_len< data_len-head_len){
			goto out;
		}
                memmove(stream.data+stream.data_len,data+head_len,data_len-head_len);
                return stream.data_len+data_len-head_len;
	}

	
out:
	if( max_new_data_len<stream.data_len){
		return -1;
	}
	memmove( new_data,data,data_len);
	return data_len;
}


