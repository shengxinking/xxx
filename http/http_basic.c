#include "http.h"

void* http_malloc(http_t* info,int len)
{
	char *p=apr_palloc(info->mp, len);
        return  p;

}
int http_char2num(char c)
{
        if( islower(c)){
                return c-'a';
        }
        if( isupper(c)){
                return c-'A';
        }

        return 26;
}
int
http_clean_str_cmp(char* dst,int dst_len,char* src,int src_len)
{
        if( dst_len<src_len){
                return -1;
        }
        if( strncasecmp(dst,src,src_len) !=0){
                return -1;
        }
        unsigned char c=dst[src_len];
        if( c != ' ' && c!=':'
                && c!= '\r' && c!='\t' && c!='\0'){

                return -1;
        }
        return 0;

}
size_t
strlcpy2(char *dst, size_t dlen, const char *src, size_t slen)
{
        register size_t n = (dlen-1) < slen ? (dlen-1) : slen;

        if ((!dst || !src))
                return 0;

        /* Copy as many bytes as will fit */

        memcpy(dst,src,n);
        /* Not enough room in dst, add NUL and traverse rest of src */

        dst[n]='\0';
        return n;               /* count does not include NUL */
}



int strlstrip(char **begin, char **end)
{
        register char *p1;
        register char *p2;

        p1 = *begin;
        p2 = *end;

        while (p1 < p2) {
                if (isspace(*p1)|| *p1==':')
                        p1++;
                else
                        break;
        }

        while (p2 > p1) {
                if (isspace(*(p2 - 1)) || *(p2 - 1) ==':')
                        p2--;
                else
                        break;
        }

        *begin = p1;
        *end = p2;

        return (p2 - p1);
}
extern char * strlstr(const char *src, size_t slen, const char *pattern, size_t plen)
{
        register const char *begin;
        register const char *end;

        begin = src;
        end = src + (slen - plen);

        do {
                if (memcmp(begin, pattern, plen) == 0)
                        return (char *)begin;
                begin++;
        } while (begin <= end);

        return NULL;
}

int http_set_str(http_t* info,x_str_t* str,char* data,int len)
{
	char* mydata=data;
	int mylen=len;
	int ret=0;
	unsigned char buf[8192];
	if( len <=0){
		return -1;
	}
	if( GET_NEED_DECODE(info->tmp_flag) != 0){
                ret=http_decode_string(buf,sizeof(buf),(unsigned char*)data,len,info->tmp_flag);
                if( ret>0){
                        mydata=(char*)&buf[0];
                        mylen=ret;
                }
        }


	str->p=http_malloc(info,mylen+1);
	str->len=mylen;
	memcpy(str->p,mydata,mylen);	
	str->p[len]='\0';
	return 0;
}
int http_set_str_no_null(http_t* info,x_str_t* str,char* data,int len,int max_len)
{
	char* mydata=data;
	int mylen=len;
	str->p=http_malloc(info,max_len+1);

	str->len=mylen;
	memcpy(str->p,mydata,mylen);	
	str->p[len]='\0';
	return 0;
}
int http_append_str(http_t* info,x_str_t* str,char* data,int len)
{
	if( str->p == NULL){
		http_set_str_no_null(info,str,data,len,4096);
		return 0;
	}
	if( str->len+len < 4096){
		memcpy(str->p+str->len,data,len);	
		str->len=str->len+len;
		str->p[str->len]='\0';
	}

	return 0;
}

local_t * http_get_local_t(http_t* info)
{
        return &info->local;
}

logic_t * http_get_logic_t(http_t* info)
{
        return &info->logic;
}
http_int_t * http_get_int_t(http_t* info)
{
        return &info->http_int;
}


http_str_t * http_get_str_t(http_t* info)
{
        return &info->str;
}
http_arg_ctrl_t * http_get_arg_t(http_t* info)
{
        return &info->arg;
}

http_file_ctrl_t * http_get_file_t(http_t* info)
{
        return &info->file;
}
int http_set_state(http_t* info,int state)
{
	logic_t* logic=http_get_logic_t(info);
	if( logic->dir ==0){
		logic->state_0=state;
	}else{
		logic->state_1=state;
	}
	return 0;
	
}
