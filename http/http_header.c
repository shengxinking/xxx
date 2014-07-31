#include "http_basic.h"
#include "http_header.h"
#include "http_table.h"
#include "http_api.h"

#define MAX_HEAD_TABLE_MIX 5
http_table_t          http_header[27][27][27][5];

int parse_null(char* value,int value_len,int offset,http_t* info)
{
	return 0;
}
int parse_unknown(char* name,int name_len,char* value,int value_len,http_t* info)
{
#if 0
	assert(data);
	http_hline_t* header=http_get_cur_hline(info);;
	int ret=0;

	if( http_config.cfg_real_flag == 1){
		http_config.org_header_len=trim_blank(http_config.org_header,http_config.org_header_len,info);
		if( header->name.len != http_config.org_header_len) {
			return 0;
		}
		if( 0== http_config.org_header_len) {
			return 0;
			
		}
                if( strncasecmp(header->name.p,http_config.org_header,http_config.org_header_len) == 0){
                        ret=get_ori_ip( data, data+data_len,info);
                }
        }

#endif
	return 0;
}
int parse_tr(char* data,int data_len,int offset,http_t* info)
{
        //{"transfer-encoding:", 0, 18, parse_trans_encoding},
        logic_t* logic=http_get_logic_t(info);
	logic->is_chunked=HTTP_LOGIC_TRUE;
        return 0;
}

int parse_us(char* data,int data_len,int offset,http_t* info)
{
        //{"User-Agent:", 0, 11, parse_http_agent},
 
	http_str_t* str=http_get_str_t(info);
	http_set_str(info,&str->agent,data,data_len);
	return 0;
}

int parse_xp(char* data,int data_len,int offset,http_t* info)
{
        //{"x-pad:", 0, 6, parse_http_xpad},
        return 0;
}
int parse_xpb(char* data,int data_len,int offset,http_t* info)
{
        //{"X-Powered-By:", 0, 13, parse_http_xpowered},
        return 0;
}
int parse_xa(char* data,int data_len,int offset,http_t* info)
{
        //{"X-AspNet-Version:", 0, 17, parse_http_xaspnetversion},

        return 0;
}
int parse_xf(char* data,int data_len,int offset,http_t* info)
{
        //{"X-Forwarded-For:", 0, 16, parse_forward},
	//parse_null(data,data_len,offset,info);
#if 0
        http_protocol_t* pro=http_get_protocol_t(info);
        char* mydata=header->value.p;
        int mylen=header->value.len;
	http_local_t* local=http_get_local_t(info);
	local->data_flag=header->flag;

	char* begin=NULL;
	char* end=NULL;
        char *q=NULL;

        q=strrlstr(mydata,mylen,",",1);
        if( q == NULL){
                q=mydata;
        }else{
                q=q+1;
        }
        int ret=http_set_str(info,&pro->x_realip,q, mydata+mylen -q);
	CHECK_MEM(ret);
	if( ret == NORMAL_ERR){
		return NORMAL_ERR;
	}

	begin=pro->x_realip.p;
	end=pro->x_realip.p+pro->x_realip.len;
       	if (strlstrip(&begin, &end) <= 0) {
		return 0;
	}
	pro->x_realip.p=begin;
	pro->x_realip.len=end-begin;
	

        struct in_addr addr;
        if (inet_aton(pro->x_realip.p, &addr) == 0) {
                pro->x_realip.len=0;
                pro->x_realip.p="\0";
        }

#endif
        return 0;
}


int parse_coo(char* data,int data_len,int offset,http_t* info)
{
#if 0
        //{"cookie:", 0, 7, parse_http_cookie},
        
        static int len= 7;//strlen("cookie:");
	http_hline_t* header=NULL;
	header=http_get_cur_hline(info);;
        http_local_t* local=http_get_local_t(info);
        local->data_flag=header->flag;
	return http_parse_cookie(data+len,data_len-len,info);
#endif
	char* mydata=data;
        int mylen=data_len;

	http_str_t* str=http_get_str_t(info);
	http_append_str(info,&str->cookie,data,data_len);

	int cnt=1;

        char* start=mydata;
        while(start<mydata+mylen){
                start=strlstr(start+1,mydata+mylen-start,";",1);
                if( start != NULL){
                        cnt++;
                }else{
                        break;
                }

        }
        info->http_int.header_cookie_num=cnt;
	
	return 0;
}
int parse_ho(char* data,int data_len,int offset,http_t* info)
{
        //{"host:", 0, 5, parse_http_host},
	http_str_t* str=http_get_str_t(info);
	http_set_str(info,&str->host,data,data_len);
	return 0;
}
int parse_lo(char* data,int data_len,int offset,http_t* info)
{
        //{"Location:", 0, 9, parse_http_location},
	http_str_t* str=http_get_str_t(info);
	http_set_str(info,&str->location,data,data_len);
	return 0;
}
int parse_re(char* data,int data_len,int offset,http_t* info)
{
	
        //{"Referer:", 0, 8, parse_http_referer},
	http_str_t* str=http_get_str_t(info);
	http_set_str(info,&str->reference,data,data_len);
	return 0;
}
int parse_ra(char* data,int data_len,int offset,http_t* info)
{
        //{"Range:", 0, 6, parse_http_range},
        char* mydata=data;
        int mylen=data_len;

	int cnt=1;

        char* start=mydata;
        while(start<mydata+mylen){
                start=strlstr(start+1,mydata+mylen-start,",",1);
                if( start != NULL){
                        cnt++;
                }else{
                        break;
                }

        }

        info->http_int.header_range_num=cnt;

	return 0;
}
typedef struct _http_content_type {
        char    name[128];
        int     name_len;
        int     id;
} http_content_type_t;

http_content_type_t http_content_type_table[] = {
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
#define	HTTP_CONTENT_TYPE_TABLE_SIZE	(sizeof(http_content_type_table)/sizeof(http_content_type_t))


int http_get_content_type(char *begin, char *end, http_t *info)
{
	unsigned int i;
	int flag=0;

	logic_t* logic=http_get_logic_t(info);
	for (i = 0; i < HTTP_CONTENT_TYPE_TABLE_SIZE; i++) {
		if( end-begin < http_content_type_table[i].name_len){
			continue;
		}
		if (!strncasecmp(begin, http_content_type_table[i].name, http_content_type_table[i].name_len)) 
		{	
			 if(begin[http_content_type_table[i].name_len] != ';'
                            && begin[http_content_type_table[i].name_len] != '\r'
                            && begin[http_content_type_table[i].name_len] != '\0'
                            && begin[http_content_type_table[i].name_len] != ' ')
			 {
				 continue;
			 }
			 logic->content_type=http_content_type_table[i].id;
			 flag = 1;

			 break;
		}
	}
	if( flag == 0){
		logic->content_type=HTTP_TYPE_NA;
	}
	return 0;
}
int get_post_boundary(char *begin, char *end, http_t *info)
{
	http_str_t* str=http_get_str_t(info);
	http_set_str(info,&str->boundary,begin,end-begin);
	local_t* local=http_get_local_t(info);
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
typedef int (*http_parse_func_t)(char *begin, char *end, http_t *info);

typedef struct _http_parse_node {
        char            token[128];
        int             token_len;
        http_parse_func_t func;
} http_parse_node_t;

http_parse_node_t parse_table[2] = {
	{"boundary=", 9, get_post_boundary},
	{"type=", 5, http_get_content_type}
};


void parse_table_parsing(char *begin, char *end, http_t *di)
{
	int i = 0;

	for (i = 0; i < 2; i++) {
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
int http_parse_content_type(char *begin, char *end, http_t *info)
{
	char *ptr = NULL;
	
	ptr = strlstr(begin, end - begin, ";",1);
	if (!ptr) {
		ptr = end;
	}

	http_get_content_type(begin, ptr, info);
	if (http_get_logic(info, HTTP_LOGIC_CONTENT_TYPE) == HTTP_TYPE_NA) {
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
int parse_cont(char* data,int data_len,int offset,http_t* info)
{	
        //{"content-type:", 0, 13, parse_content_type},

	http_str_t* str=http_get_str_t(info);
	http_set_str(info,&str->content_type,data,data_len);

	return http_parse_content_type(data,data+data_len,info);
	return 0;
}
int parse_conn(char* data,int data_len,int offset,http_t* info)
{	
        //{"Connection:", 0, 11, parse_http_conn},
        logic_t* logic=http_get_logic_t(info);
	
	if( strncasecmp(data,"close",5) == 0){ 
		if( logic->dir == 1 && http_get_int(info,HTTP_INT_CONTENT_LEN) <=0){
			logic->is_closed = HTTP_LOGIC_TRUE;
		}
	}else{
               	logic->is_closed = HTTP_LOGIC_FALSE;
       	}
	return 0;
}
int parse_cone(char* data,int data_len,int offset,http_t* info)
{	
#if 0
        //{"Content-Encoding:", 0, 17, parse_http_content_encoding},
        logic_t* logic=http_get_logic_t(info);
	http_hline_t* header=NULL;

	header=http_get_cur_hline(info);;

	if( strlstr(header->value.p,header->value.len,"gzip",4) != NULL){
               	logic->is_gzip= HTTP_LOGIC_TRUE;
               	if( strlstr(header->value.p,header->value.len,";",1) != NULL){
                      	logic->is_only_gzip=HTTP_LOGIC_FALSE;
               	}else{
			logic->is_only_gzip=HTTP_LOGIC_TRUE;
		}

       		logic->is_cenc=HTTP_LOGIC_TRUE;
	}

#endif
	return 0;
}

int parse_conl(char* data,int data_len,int offset,http_t* info)
{	
	//{"content-length:", 0, 15, parse_content_length},
	char buf[24];
	if( (unsigned int)data_len > sizeof(buf)-1){
		return -1;
	}
	memcpy(buf,data,data_len);
	buf[data_len]='\0';
	int content_len=strtol(buf,NULL,10);
	int ret=0;
	http_int_t* http_int=http_get_int_t(info);
	if( ret <0){
		return ret;
	}
	http_int->content_len=content_len;
	
	return 0;

}

int parse_ace(char* data,int data_len,int offset,http_t* info)
{
#if 0
	http_protocol_t* pro=http_get_protocol_t(info);
	http_hline_t* header=http_get_cur_hline(info);;
        http_local_t* local=http_get_local_t(info);
        local->data_flag=header->flag;
       	int ret=http_set_str(info,&pro->accept_encoding,header->value.p, header->value.len);
	CHECK_MEM(ret);
#endif
	return 0;
	
}

int parse_ac(char* data,int data_len,int offset,http_t* info)
{
#if 0
        //{"Accept:", 0, 7, parse_http_accept},
	assert(data);
	logic_t* logic=http_get_logic_t(info);
	http_hline_t* header=http_get_cur_hline(info);;
        http_local_t* local=http_get_local_t(info);
        local->data_flag=header->flag;


        char* begin=data;
	char* end=data+data_len;
	begin += 7;
       	if (strlstrip(&begin, &end) <= 0) {
		goto out;
        }

        if (NULL != strlcasestr(begin,end-begin,"application/rpc",15 )) {
        	logic->is_rpc = HTTP_LOGIC_TRUE; 
	}
#endif

	return 0;
}


int parse_au(char* data,int data_len,int offset,http_t* info)
{
        //{"authorization:", 0, 14, parse_http_authorization},
	
	int ret=0;
	char* begin=data;
	char* end=data+data_len;
	http_str_t* str=http_get_str_t(info);
	local_t* local=http_get_local_t(info);
        if ((strncasecmp("Digest", data, 6)) == 0 || \
            (strncasecmp("Negotiate", data, 9)) == 0 || \
            (strncasecmp("NTLM", data, 4)) == 0) {
		local->data_flag=0;
                ret=http_set_str(info,&str->auth,data, data_len );
                goto out;
        }

        if ((strncasecmp("Basic", begin, 5)) != 0) {
                goto out;
        }

	int jump_len=5;//strlen("basic");
        begin += jump_len;
        if (strlstrip(&begin, &end) <= 0) {
                goto out;
        }

	http_set_str(info,&str->auth,begin, end - begin );

        int flag = b64_decode((char *)str->auth.p, \
                (unsigned char *)str->auth.p,  str->auth.len);
	if( flag >0){
		str->auth.len=flag;
		str->auth.p[flag]='\0';
	}

        begin = str->auth.p;
        end = begin + str->auth.len;

        if (strlstrip(&begin, &end) <= 0) {
                goto out;
        }

        char *tag = begin;


        while (end > tag) {
                if (*tag == ':') {
                        break;
                }
                tag++;
        }

        if (end > tag ) {
                ret=http_set_str(info,&str->auth_user,begin, tag-begin);
		
                ret=http_set_str(info,&str->auth_pass,tag+1, end-tag-1);
        }
out:
	return 0;
}
int parse_set(char* data,int data_len,int offset,http_t* info)
{
	return 0;
}

int
http_set_header_table(char* name, http_func_t cb, int num)
{
        http_table_t *table=NULL;
        int len=strlen(name);
        table=&http_header[http_char2num(name[0])][http_char2num(name[1])][http_char2num(name[2])][num];
        strcpy(table->key,name);
        table->keylen=len;
        table->func=cb;
        return 0;

}



int http_init_header_table()
{
	int i=0;
	int j=0;
	int k=0;
	int m=0;
	char* name=NULL;
	for(i=0;i<27;i++){
		for(j=0;j<27;j++){
			for(k=0;k<27;k++){
				for(m=0;m<10;m++){
					http_header[i][j][k][m].func=parse_null;
					http_header[i][j][k][m].keylen=0;
				}
			}
		}
	}
        //{"authorization:", 0, 14, parse_http_authorization},

	name="authorization";
	http_set_header_table(name,parse_au,0);
        //{"Accept:", 0, 7, parse_http_accept},
        //{"Accept-Encoding:", 0, 16, parse_http_accept_encoding},
	name="Accept";
	http_set_header_table(name,parse_ac,0);
	name="Accept-Encoding";
	http_set_header_table(name,parse_ace,1);

        //{"content-length:", 0, 15, parse_content_length},
        //{"content-type:", 0, 13, parse_content_type},
        //{"Connection:", 0, 11, parse_http_conn},
        //{"Content-Encoding:", 0, 17, parse_http_content_encoding},
        name="content-length";
	http_set_header_table(name,parse_conl,0);

        name="content-type";
	http_set_header_table(name,parse_cont,1);

        name="Connection";
	http_set_header_table(name,parse_conn,2);

        name="content-Encoding";
	http_set_header_table(name,parse_cone,3);

        //{"cookie:", 0, 7, parse_http_cookie},
        name="cookie";
	http_set_header_table(name,parse_coo,0);

        //{"host:", 0, 5, parse_http_host},
        name="host";
	http_set_header_table(name,parse_ho,0);

        //{"Location:", 0, 9, parse_http_location},
        name="location";
	http_set_header_table(name,parse_lo,0);

        //{"Referer:", 0, 8, parse_http_referer},
        name="Referer";
	http_set_header_table(name,parse_re,0);
        //{"Range:", 0, 6, parse_http_range},
        name="range";
	http_set_header_table(name,parse_ra,0);

        //{"Set-Cookie2:", 0, 12, parse_http_set_cookie2},
        //{"Set-Cookie:", 0, 11, parse_http_set_cookie},
        //{"Server:", 0, 7, parse_http_server},
	name="set-cookie";
	http_set_header_table(name,parse_set,0);


        //{"transfer-encoding:", 0, 18, parse_trans_encoding},
	name="transfer-encoding";
	http_set_header_table(name,parse_tr,0);

        //{"User-Agent:", 0, 11, parse_http_agent},
	name="User-Agent";
	http_set_header_table(name,parse_us,0);

        //{"x-pad:", 0, 6, parse_http_xpad},
        //{"X-Powered-By:", 0, 13, parse_http_xpowered},
        name="x-pad";
	http_set_header_table(name,parse_xp,0);
        name="x-powered-by";
	http_set_header_table(name,parse_xpb,1);

        //{"X-AspNet-Version:", 0, 17, parse_http_xaspnetversion},
        name="X-AspNet-Version";
	http_set_header_table(name,parse_xa,0);
        //{"X-Real-IP:", 0, 10, parse_real_ip},
        //{"X-Forwarded-For:", 0, 16, parse_forward},
        name="X-Forwarded-For";
	http_set_header_table(name,parse_xf,0);
        //{"X-Client-DN:", 0, 12, parse_subject},
        //{"X-Client-Cert:", 0, 14, parse_cert},
	return 0;
}
int http_match_header_table(char *name,int name_len, char *value, int value_len, http_t *info)
{
	http_table_t *table= http_header[http_char2num(name[0])][http_char2num(name[1])][http_char2num(name[2])];
	int i=0;
	
	int ret=0;
	for(i=0;i<MAX_HEAD_TABLE_MIX;i++){
		if(table[i].keylen==0){	
			parse_unknown(name,name_len,value,value_len,info);
			break;
		}
		if( table[i].keylen != name_len){
			continue;
		}
		if( http_clean_str_cmp(name,name_len,table[i].key,table[i].keylen) != 0){
			continue;
		}
		ret=table[i].func(value,value_len,0,info);
		break;
		
	}
	return 0;
	
}
int http_myparse_method(char* data,int len,http_t* info)
{
	static int ver_len=8;//strlen("http/1.1");
	char *begin=data;
	char *end=data+len-ver_len-1;
	int mylen=len;

	
	int ret=0;
	strlstrip(&begin,&end);
	mylen=end-begin;

	
	http_str_t* str=http_get_str_t(info);

	end=begin+mylen;	
	strlstrip(&begin,&end);
	mylen=end-begin;

	int flag=info->tmp_flag;
	info->tmp_flag=0;
	ret=http_set_str(info,&str->raw_url,begin,mylen);
	info->tmp_flag=flag;
	ret=http_set_str(info,&str->url_and_arg,begin,mylen);
	char* tmp=strlstr(data,len,"?",1);
	if ( tmp== NULL){
		ret=http_set_str(info,&str->url,begin,mylen);
	}else{
		ret=http_set_str(info,&str->url,begin,tmp-begin);
	}
	if( tmp != NULL){
		tmp++;
		_http_parse_args(info,tmp,end-tmp,1);
		http_arg_ctrl_t* arg=http_get_arg_t(info);
		arg->cur=NULL;

	}
	
		
	return 0;
}


int parse_method(char* data,int data_len,int offset,http_t* info)
{
	char* begin=data;
	char* end=NULL;
	int ret=0;
	end=strlstr(data,data_len," ",1);
	if( end== NULL){
		return 0;
	}
	strlstrip(&begin,&end);
	data_len=data_len-(begin-data);
        http_table_t *table= &http_methods[http_char2num(begin[0])][http_char2num(begin[1])][0];
        int i=0;

        logic_t* logic=http_get_logic_t(info);
        for(i=0;i<MAX_METHOD_TABLE_MIX;i++){
           if(table[i].keylen==0){
				http_method_null(begin,end-begin,offset,info);
				logic->method=HTTP_METHOD_NA;
				ret=http_myparse_method(end+1,begin+data_len-end-1,info);
                break;
			}
            
		    if( table[i].keylen != end-begin){
                        continue;
			}
		
			if( http_clean_str_cmp(begin,end-begin,table[i].key,table[i].keylen) != 0){
				continue;
			}
			
			logic->method=table->value;

		
			if( data_len >table[i].keylen+1){
				ret=http_myparse_method(begin+table[i].keylen+1,data_len-table[i].keylen-1,info);
			}else{
				ret=-1;
			}
                //table[i].func(data,len,offset,info);
                break;
        }

	return ret;
}

int parse_retcode(char* data,int data_len,int offset,http_t* info)
{
	http_int_t* http_int=http_get_int_t(info);

	char* begin=data;
	char* end=NULL;
	
	static int min_len=8;//strlen("http/1.1/");
	if( data_len <8){
		return -1;
	}

        begin += min_len;
	

        http_int->retcode = atoi(begin);

	http_str_t* str=http_get_str_t(info);
	end=strlstr(begin+1,data+data_len-begin," ",1);
	if( end!= NULL){
        	http_set_str(info,&str->retcode,begin+1,end-begin);
	}

	return 0;
}


int http_parse_line(http_t* info,char* begin,char* split,char* end,int dir,int num)
{
	int ret=0;
	
	http_int_t* http_int=http_get_int_t(info);

	
	if( num == 0){
		if( dir==0){
			ret=parse_method(begin,end-begin,0,info);
			return ret;
		}else{
			ret=parse_retcode(begin,end-begin,0,info);
			return ret;
        	}
	}
	if( split == NULL){
		return 0;
	}
	if( end-begin> http_int->max_header_line_len){
		http_int->max_header_line_len=end-begin;
	}
	char* tmp=split;
	strlstrip(&begin,&tmp);
	int name_len=tmp-begin;

	tmp=split;
	strlstrip(&tmp,&end);
	int value_len=end-tmp;
	
	ret=http_match_header_table(begin,name_len,tmp, value_len,  info);
        return ret;
}

