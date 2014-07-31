#ifndef _HTTP_BASIC_H
#define _HTTP_BASIC_H
#include "http.h"

local_t * http_get_local_t(http_t* info);

logic_t * http_get_logic_t(http_t* info);
http_int_t * http_get_int_t(http_t* info);
http_arg_ctrl_t * http_get_arg_t(http_t* info);
http_arg_t* http_get_empty_arg(http_t* info);
http_file_t* http_get_empty_file(http_t* info);
http_file_ctrl_t* http_get_file_t(http_t* info);





size_t strlcpy2(char *dst, size_t dlen, const char *src, size_t slen);

http_str_t * http_get_str_t(http_t* info);
int http_char2num(char c);
int strlstrip(char **begin, char **end);
int b64_encode(u_char const *src, size_t srclength, char *target, size_t targsize);
int b64_decode(char const *src, u_char *target, size_t targsize);



int http_clean_str_cmp(char* dst,int dst_len,char* src,int src_len);
extern char * strlstr(const char *src, size_t slen, const char *pattern, size_t plen);
int http_set_str(http_t* info,x_str_t* str,char* data,int len);
int http_append_str(http_t* info,x_str_t* str,char* data,int len);

int _http_parse_args(http_t* info,char* data,int data_len,int in_url);

int http_set_state(http_t* info,int state);


int http_parse_form(char* data,int len,http_t* info);
int http_append_arg(char* value,int value_len,http_t* info,http_arg_t* item);
http_file_t * http_get_file(http_t* info,int num);



int http_dump_str(http_t* info);
int http_dump_int(http_t* info);
int http_dump_logic(http_t* info);
int http_dump_arg(http_t* info);
int http_dump_file(http_t* info);



#define HTTP_CHUNK_SIZE 0
#define HTTP_CHUNK_FIN 1




#endif
