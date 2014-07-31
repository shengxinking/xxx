/**
 *	@file	http_parse.h
 *
 *	@brief	The HTTP protocol parse API/structure defines.
 *
 * 	@author	Xinhui Wang.
 */

#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "stdext.h"
#include "fcrypto.h"
#include "http_decode.h"
#include "protutil.h"

#include "aprutil/apr_pools.h"
#include "aprutil/apr_tables.h"

/* the parameter need ignored in http parse */
#define HTTP_SM_CKNAME		"cookiesession1"
#define HTTP_PARAM_NAME		"cookiesession8341"
#define WAF_PREFIX_NEW      "cookiesession2"
#define HTTP_REDIECT_PARAM	"redirect491"
#define HTTP_REASON_PARAM	"reason747sha"
#define HTTP_REWRITE_PARAM	"rewrite491"
#define HTTP_IGNORE_PARAM_NUM	6	//must be num+1

/* http main state */
enum {
	HTTP_STATE_INIT = 0,
	HTTP_STATE_HEAD,
	HTTP_STATE_BODY,
	HTTP_STATE_NONE,
	HTTP_STATE_ERROR, 
	HTTP_STATE_NUM,
};

/* http version */
enum {
	HTTP_VER_NA = 0,
	HTTP_VER_11,
	HTTP_VER_10,
	HTTP_VER_09,
	HTTP_VER_12,
};

typedef struct _http_config{
	int		decode_flag;
	int		amf;
	int		cfg_real_flag;
	char		org_header[48];
	int		org_header_len;
	int 		ip_location;
	int		remove_para_flag;
        int		remove_8341;
	int		header_max;
	int		args_max;
	int             upfile_count;  
        int             amf_cache_size;

} http_config_t;

extern http_config_t http_config;
typedef struct _http_str{
	char		*p;
	int		len;
} http_str_t;

typedef struct _http_combuf{
        char		*buf;
        u_int32_t	current;
        u_int32_t	max_len;
} http_combuf_t;

typedef struct _http_header_line{
	http_str_t	name;
	http_str_t	value;
}http_header_line_t;

typedef struct _http_hline{
	int		begin;
	int		split;
	int		len;
	int		flag;
	http_str_t	name;
	http_str_t	value;
} http_hline_t;

typedef struct _http_hctrl{
        http_combuf_t	buf;
	http_hline_t	*cur;
	apr_array_header_t *array;
        short		num;
        short		max_num;
	int 		extern_flag;
}http_hctrl_t;

typedef struct _http_file{
#define HTTP_FILE_MAX_LEN 512
	http_str_t	name;
	http_str_t	value;
	int complete_flag;
	int raw_file_len;
	int illegal_filesize_logged; //logged already or not?
}http_file_t;

typedef struct _http_fctrl{
	http_file_t*	cur;
	apr_array_header_t* array;
        short		num;
        short		new_num;
        short		max_num;
}http_fctrl_t;

typedef struct _http_av{
	http_str_t	name;
	http_str_t	value;
} http_av_t;

typedef struct _http_avctrl{
	http_av_t	*cur;
	apr_array_header_t* array;
        short		num;
        short		max_num;
}http_avctrl_t;

typedef struct _http_arg{
	http_str_t	name;
	http_str_t	value;
	int complete_flag;
	int		arg_flag;
	int		name_flag;
	int		value_flag;
} http_arg_t;

typedef struct _http_argctrl{
        http_combuf_t	buf;
	http_arg_t	*cur;
	apr_array_header_t* array;
        short		num;
	short		new_num;
        short		max_num;
	int 		extern_flag;
}http_argctrl_t;

typedef struct _http_cookie{
	http_str_t	name;
	http_str_t	value;
	http_str_t	path;
	http_str_t	domain;
	http_str_t	expire;
} http_cookie_t;

typedef struct _http_ckctrl{
	http_cookie_t	*cur;
	apr_array_header_t *array;
        short		num;
        short		max_num;
} http_ckctrl_t;

typedef struct _http_amf{
	http_str_t	name;
	http_str_t	value;
	http_str_t	class_name;
} http_amf_t;

typedef struct _http_amfctrl{
        http_combuf_t	buf;
	http_amf_t	*cur;
	apr_array_header_t* array;
        short		num;
        short		max_num;
} http_amfctrl_t;

typedef struct _http_local{
	int		sub_status;
	int		data_flag;
	int		clen_isset_0;
	int		clen_isset_1;
	int		unrcv_len_0;
	int		unrcv_len_1;
	int		tenc_0; // transfer incoding
	int		tenc_1; // transfer incoding
	int		unrcv_cklen; //un recv chunk len
	int		chunk_flag;
	char		cache_buf[512];
	int		cache_buflen;
	int		form_status;
	int		form_flag;
	int		subject_flag;
	int		cert_flag; 
	int		realip_flag;
	int		abs_url_flag;
	int 		header_buf_error;
	int 		args_buf_error;
	int 		amf_error;
	int 		file_num_error;

#define LABEL_STATE_BEGIN 1
#define LABEL_STATE_RUN   2
#define LABEL_STATE_NULL  3

	int 		myform_label_num;
	int 		myform_label_state;
	int 		null_file;
	int 		form_header_state;
	int 		boundary_cnt;
	int 		last_len;
	int		have_8341;
	int		force_decode;
} http_local_t;

typedef struct _http_logic{
	int		state_0;	/* request state */
	int		state_1;	/* response state */
	int		dir;		/* direct: request/response */
        short		is_wafrdt;	/* waf redirect */
        short		is_wafsid;	/* wafsid */
        short		is_chunked;	/* trunked */
        short		is_rpc;		/* rpc */
        short		is_dropargs;	/* dropped args */
        short		is_ver_11;	/* HTTP/1.1 */
        short		is_gzip;	/* gzip method */
        short		is_only_gzip;	/* only gzipped */
        short		is_closed;	/* server closed */
        short		is_cenc;	/* content encoding */
	short		method;		/* method */
        short		version_0;	/* request http version */	
        short		version_1;	/* response http version */
        short		ctype_0;	/* request content-type */
        short		ctype_1;	/* response content-type */
} http_logic_t;

typedef struct _http_int{
	u_int16_t	range;		/* Request Range number */
	u_int16_t	retcode;	/* Response code */
	u_int16_t	nallarg;	/* number of all argument */
	u_int16_t	nurlarg;	/* number of URL argument */
	u_int16_t	urlarg_len;	/* URL argument length */
	u_int16_t	allarg_len;	/* all argument length */

	u_int16_t	ncookie_0;	/* number of cookie */
	u_int16_t	nhline_0;	/* number of headline */
	u_int16_t	max_hllen_0;	/* max header line length */
	u_int16_t	blen_0;		/* Request Body length */
	u_int32_t	clen_0;		/* Request Content-Length */
	u_int16_t	hlen_0;		/* Request Header length */
	u_int16_t	norn_hlen_0;	/* strip \r\n header length */

	u_int16_t	ncookie_1;	/* number of cookie */
	u_int16_t	nhline_1;	/* number of headline */
	u_int16_t	max_hllen_1;	/* max header line length */
	u_int16_t	blen_1;		/* Response Body length */
	u_int32_t	clen_1;		/* Response Content-Length */
	u_int16_t	hlen_1;		/* Response Header length */
	u_int16_t	norn_hlen_1;	/* strip \r\n header length */
} http_int_t;

typedef struct _header_protocol{
	http_str_t	raw_fullurl;	/* origin full URL, include args */
	http_str_t	raw_url;	/* the URL without args */
	http_str_t	fullurl;	/* decoded URL, include args */
	http_str_t	url;		/* the decode URL without args */
	http_str_t	pathurl;	/* URL decode path like ../../ */
	http_str_t	raw_host;	/* Host */
	http_str_t	host;		/* decoded Host */
	http_str_t	agent;		/* User-Agent value */
	http_str_t	xff;		/* X-Forwarded-For value */
	http_str_t	xpad;		/* X-Pad value */
	http_str_t	x_realip;	/* X-Real-IP value */
	http_str_t	x_power;	/* X-Power value */
	http_str_t	reference;	/* Reference value */
	http_str_t	auth;		/* Auth value */
	http_str_t	auth_user;	/* Auth-Username value */
	http_str_t	auth_pass;	/* Auth-Password value */
	http_str_t	asp_net;	/* Asp-Net value */
	http_str_t	js_arg;		/* Javascript argument */
	http_str_t	origin_ip;	/* the origin IP */
	http_str_t	location;	/* Location value */
	http_str_t	location_url;	/* Location URL value */
	http_str_t	location_host;	/* Location Host value */
	http_str_t	server;		/* Server value */
	http_str_t	retcode;	/* Ret-Code str */
	http_str_t	ctype_0;	/* The content-Type str */
	http_str_t	ctype_1;	/* The content-Type str */
	http_str_t	boundary;	/* Post boundary */
	http_str_t	wafsid;		/* FortiWAF arg */
	http_str_t	accept_encoding;/* Accept encoding */
} http_protocol_t;

typedef struct _http_para{
	http_local_t	local;
	http_logic_t	logic;
	http_int_t	hint;
	http_protocol_t	protocol;
	http_hctrl_t	header_0;
	http_hctrl_t	header_1;
	http_ckctrl_t	cookie_0;
	http_ckctrl_t	cookie_1;
	http_argctrl_t	arg;
	http_fctrl_t	file;
	http_avctrl_t	av;
	http_amfctrl_t	amf;
} http_para_t;

typedef struct _http_info{
	http_config_t	cfg;		/* config */
	int		init_flag;
	int	error_line;
	int	mem_error;
	int	ver_error;
	http_para_t	*http;		/* http parameters */
	char		*data;		/* the data */
	void		*ssn;		/* the session */
	apr_pool_t 	*mp;		/* apr_pool for http parse */
	int 		error;
} http_info_t;

/* http BM string pattern and jump table */
typedef struct _http_bmpat {
	char		*name;
	int		len;
	u_int32_t	jump_tab[256];
} http_bmpat_t;

typedef struct _http_ignore_param {
	http_bmpat_t	params[HTTP_IGNORE_PARAM_NUM];
	int		init_flag;
} http_ignore_param_t;

extern int  
http_set_state(http_info_t *info,int status);

/* bool value */
#define	HTTP_LOGIC_FALSE	0
#define HTTP_LOGIC_TRUE		1

/* gzip method */
#define HTTP_GZIP_NA		0
#define HTTP_ONLY_GZIP		1
#define HTTP_NOT_ONLY_GZIP	2
/**
 *	Get logic protocol variable, the index is HTTP_LOGIC_XXXX(see above).
 *
 *	Return LOGIC.
 */
extern int 
http_get_logic(struct _http_info *info, int index);

/**
 *	Get int protocol variable, the index is HTTP_INT_XXXX(see above).
 *
 *	Return int.
 */
extern int 
http_get_int(struct _http_info *info, int index);


/**
 *	Get string protocol variable, the index is HTTP_STR_XXXX(see above).
 *
 *	Return string if success, "\0" if not found.
 */
extern char * 
http_get_str(struct _http_info *info,int index);

extern int 
http_get_strlen(struct _http_info *info,int index);

enum {
	HTTP_HEADBUF,	
	HTTP_ARGBUF,
	HTTP_AMFBUF,
};		
extern char * 
http_get_buf(struct _http_info *info,int index);

extern int 
http_get_buflen(struct _http_info *info,int index);
/**
 *	the follow func poll the header/cookie/arg/file/av 
 *
 */

int 
http_get_header_line_cnt(http_info_t* info);

extern http_header_line_t * 
http_get_header_line(http_info_t* info,int num);

extern int 
http_get_file_cnt(http_info_t* info);

extern http_file_t * 
http_get_file(http_info_t* info,int num);

extern int 
http_get_av_cnt(http_info_t* info);

extern http_av_t * 
http_get_av(http_info_t* info,int num);

extern int 
http_get_arg_cnt(http_info_t* info);

extern http_arg_t * 
http_get_arg(http_info_t* info,int num);

extern int 
http_get_cookie_cnt(http_info_t* info);

extern http_cookie_t * 
http_get_cookie(http_info_t* info,int num);

extern int 
http_get_amf_cnt(http_info_t* info);

extern http_amf_t * 
http_get_amf(http_info_t* info,int num);

extern int 
http_clear_new_args(http_info_t *info);

extern int 
http_get_new_arg_cnt(http_info_t *info);

extern http_arg_t * 
http_get_new_arg(http_info_t* info,int num);

extern int 
http_clear_new_file_flag(http_info_t *info);
extern int 
http_get_new_file_cnt(http_info_t *info);

extern http_file_t * 
http_get_new_file(http_info_t* info,int num);

extern int 
http_parse_av(char* data,int data_len,http_info_t* info);
extern int
http_parse_amf(char* data,int data_len,http_info_t* info);


extern int 
http_global_init();

extern int 
http_global_clean();
int 
http_get_hline_cnt(http_info_t* info);

extern http_hline_t * 
http_get_hline(http_info_t* info,int num);


extern int 
http_init(http_info_t* info,void* sess);

extern int 
http_clean(http_info_t* info);

extern http_protocol_t * 
http_get_protocol_t(http_info_t* info);

extern http_hctrl_t * 
http_get_header_t(http_info_t* info);

extern http_fctrl_t * 
http_get_file_t(http_info_t* info);

extern http_avctrl_t * 
http_get_av_t(http_info_t* info);

extern http_argctrl_t * 
http_get_arg_t(http_info_t* info);

extern http_ckctrl_t * 
http_get_cookie_t(http_info_t* info);

extern http_hline_t * 
http_get_cur_hline(http_info_t* info);

extern http_int_t * 
http_get_int_t(http_info_t* info);

extern http_local_t * 
http_get_local_t(http_info_t* info);

extern http_logic_t * 
http_get_logic_t(http_info_t* info);

extern http_config_t * 
http_get_config_t(http_info_t* info);

extern http_amfctrl_t * 
http_get_amf_t(http_info_t* info);

#define	HTTP_HEADER_OVERSIZE	0x01
#define HTTP_ARGBUF_OVERSIZE	0x02
#define HTTP_FILE_NUM_OVERSIZE	0x04
#define HTTP_ERROR_NUM 3 //add by sxwang, Malfromed number
int http_parse(char* data,int data_len,http_info_t* info,int dir);
int http_reset_response(http_info_t* info);

int http_get_error(http_info_t* info);
int http_is_critical_error(http_info_t* info);
int http_get_normal_error(http_info_t* info);
int http_get_limit_error(http_info_t* info);


#endif

