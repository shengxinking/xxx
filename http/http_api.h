#ifndef _HTTP_API_H
#define _HTTP_API_H
#include "http.h"
//#include "xlog.h"
enum {
	HTTP_STR_RAW_URL,
	HTTP_STR_URL,			/* decoded URL without ARGS */
	HTTP_STR_URL_AND_ARG,
	HTTP_STR_HOST,
	HTTP_STR_COOKIE,
	HTTP_STR_AGENT,
	HTTP_STR_AUTH,
	HTTP_STR_USER,
	HTTP_STR_PASSWD,
	HTTP_STR_REFER,
	HTTP_STR_LOCATION,
	HTTP_STR_CONTENT_TYPE,	
	HTTP_STR_RETCODE,
};
enum {
	/* bool variable */
	HTTP_LOGIC_STATE,
	HTTP_LOGIC_METHOD,
	HTTP_LOGIC_IS_CLOSED,
	HTTP_LOGIC_CONTENT_TYPE,
	HTTP_LOGIC_IS_CHUNKED,
};
enum {
	HTTP_INT_RET_CODE,
	HTTP_INT_HEADER_LEN,
	HTTP_INT_CONTENT_LEN,
	HTTP_INT_MAX_HEADER_LINE_LEN,
	HTTP_INT_BODY_LEN,
	HTTP_INT_URL_ARG_LEN	,
	HTTP_INT_URL_ARG_CNT,
	HTTP_INT_REQUEST_LEN,
	HTTP_INT_HEADER_LINE_NUM,
	HTTP_INT_COOKIE_NUM,
	HTTP_INT_RANGE_NUM,
	
};


int http_get_logic(http_t *info, int index);
int http_get_int(http_t *info, int index);
char* http_get_str(http_t *info, int index);
int http_get_str_len(http_t *info, int index);

enum {
        HTTP_TYPE_NA ,          /* unknowed type */
        HTTP_TYPE_MR,           /* mime */
        HTTP_TYPE_SOAP,         /* soap */
        HTTP_TYPE_XML,          /* xml */
        HTTP_TYPE_MULTIFORM,    /* multipart/form */
        HTTP_TYPE_HTML,         /* html */
        HTTP_TYPE_FORMENC,      /* */
        HTTP_TYPE_PLAIN,        /* plain */
        HTTP_TYPE_AMF,          /* amf */
        HTTP_TYPE_CSS,          /* css */
        HTTP_TYPE_JS,           /* javascript */
        HTTP_TYPE_MIX_XML,      /* mixed xml */
        HTTP_TYPE_COMMON_JS,    /* common js */
        HTTP_TYPE_TEXT_JS,      /* text js */
        HTTP_TYPE_RPC,          /* rpc */
        HTTP_TYPE_RSS_XML,      /* rss xml */
        HTTP_TYPE_XHTML_XML,    /* xhtml/xml */
        HTTP_TYPE_MESSAGE,    /* xhtml/xml */
};



extern int
http_get_file_cnt(http_t* info);

#endif



