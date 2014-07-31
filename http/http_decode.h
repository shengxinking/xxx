#ifndef _HTTP_DECODE_H
#define	_HTTP_DECODE_H

#define MAX_DECODE_LEN 16384
#define ONLY_ONE_DECODE 0
#define CIC_DECODE 1

#define GET_BIT(x,n) (x&(0x01<<n))
#define SET_BIT(x,n) (x=x|(0x01<<n))

#define GET_NEED_DECODE(flag) (GET_BIT(flag,0)||GET_BIT(flag,1)||GET_BIT(flag,2))
#define GET_ALL_DECODE(flag) (GET_BIT(flag,0)&&GET_BIT(flag,1)&&GET_BIT(flag,2))

#define GET_NEED_URL_DECODE(flag) GET_BIT(flag,0)
#define SET_NEED_URL_DECODE(flag) SET_BIT(flag,0)

#define GET_NEED_HTML_DECODE(flag) GET_BIT(flag,1)
#define SET_NEED_HTML_DECODE(flag) SET_BIT(flag,1)

#define GET_NEED_ESCAPE_DECODE(flag) GET_BIT(flag,2)
#define SET_NEED_ESCAPE_DECODE(flag) SET_BIT(flag,2)

#define HTTP_CHUNK_SIZE 0
#define HTTP_CHUNK_FIN 1

#if 0
/*************************************************************
	Function: http_get_url_core 
	Description: 
 		remove spaces and tabs from the url begin and end. 
	Return:
		error: -1
		success: the url_out stirng length
**************************************************************/
extern int http_get_url_core(unsigned char **url_out,  unsigned char *url_in, int url_in_len);
#endif

/*************************************************************
	Function: http_decode_url
	Description:
		decode the url
	Params:
		out_string: 		the decoded url
		max_url_out_len:	the max length of decoded url 
		in_string:		the url to be decoded
		in_string_len:		the length of the url to be decoded
		decoce_flag:		ONLY_ONE_DECODE or CIC_DECODE
	Return:		
		error: -1
		success: the length of out_string

**************************************************************/
extern int http_decode_string(unsigned char *out_string, int max_url_out_len, unsigned char *in_string, int in_string_len,int data_flag);

/**************************************************************

  Function:	http_decode_path

  Description:	copy path to collapsed path.

  e.g:  	"////////////"	 -->	 "/"
      		"/./"		 --> 	 "/"
	        "/aaa/../"	 -->	 "/"      
	        "\"		 -->	 "/"
 		passed path must be anchored with a '/'

  Return:	-1	fault
		0	success

abc/abc/aaabbb
**************************************************************/
extern int http_decode_path(u_int8_t *s, int32_t s_len);

#if 0
/*************************************************************
 *Description: remove comments in "input"
 *
**************************************************************/
extern int http_msre_fn_replace_comments_execute(unsigned char *input, long int input_len);


/*************************************************************
 *Description: lower case of the string input
 *************************************************************/
extern int http_msre_fn_lowercase_execute(unsigned char *input, long int input_len);

//char *encode_str( const char *src, char *des );
 
#endif

#endif 
