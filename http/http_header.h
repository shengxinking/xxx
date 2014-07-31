#ifndef _HTTP_HEADER_H_
#define _HTTP_HEADER_H_
#include "http.h"
int http_parse_line(http_t* info,char* begin,char* split,char* end,int dir,int num);

#endif
