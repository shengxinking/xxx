#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include  "http_decode.h"
#include  "http_basic.h"
#define MAX_STRING_LEN MAX_DECODE_LEN
#define NBSP 32

#ifndef MIN             
#define MIN(a, b)       ((a) > (b) ? (b) : (a))
#endif //MIN   

#define NO_PERCENT		0
#define PERCENT_XX		1
#define PERCENT_U00XX		2
#define PERCENT_XYY		3
#define IGNORANCE_XX		4
#define IGNORANCE_U00XX		5
#define IGNORANCE_XYY		6
#define PERCENT_UFFXX		7


static void _http_print_buf2(u_int8_t *buf, int32_t nbuf, u_int32_t column)
{
        int32_t i, j;
        int32_t col;
        u_int8_t ch;

        if (column == 0)
                col = 16;
        else
                col = column;

        for (i = 0; i < nbuf; i++) {
                printf ("%x%x ", buf[i] >> 4, buf[i] & 0xf);
                if ((i + 1) % col == 0) {
                        for (j = 0; j < col; j++) {
                                ch = *(buf + i + j - (col-1));
                                printf ("%c", isprint (ch) ? ch : '.');
                        }
                        printf ("\n");
                }
        }
        for (j = 0; j < col - i % col; j++)
                printf ("   ");
        for (j = 0; j < i % col; j++) {
                ch = *(buf + i + j - i % col);
                printf ("%c", isprint (ch) ? ch : '.');
        }
        printf ("\n");

        return;
               
}

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
int http_decode_path(u_int8_t *s, int32_t s_len)
{
        int id;                 /* index of destination string */
        int is;                 /* index of source string */
        int slashseen;  /* have we seen a slash */
        //int ls = 0;                 /* length of source string */
        int ls = s_len + 1;                 /* length of source string */

        /* let "is" ranges from the first u_int8_t to '\0' of string "s"*/
        //ls = strlen ((char *)s) + 1;

	if( s_len >MAX_DECODE_LEN || s_len<=0){
		return  0;
	}
        slashseen = 0;

        for(is = 0, id = 0; is < ls; /*is ++*/){
                /* thats all folks, we've reached the end of input */
                if(s[is] == '\0'){
                        s[id] = '\0';
                        break;
                }
                /* previous u_int8_chacter was a / */
                if(slashseen){
                        if(s[is] == '/' || s[is] == '\\'){
                                is ++;
                                continue;       /* another slash, ignore it */
                        }else
                                slashseen = 0;
                }else if(s[is] == '/' || s[is] == '\\'){
                        /* we see a '/', just copy it and try again */
                        slashseen = 1;
                        s[id] = '/';
                        id ++;
                        is ++;
                        continue;
                }

                /* /./ seen */
                if(s[is] == '.' && (s[is + 1] == '/' || s[is + 1] == '\\')){
                        slashseen = 1;
                        is += 2;
                        continue;
                }

                /* XXX/. seen */
                if(s[is] == '.' && s[is + 1] == '\0'){
                        if(id > 1) /* in case of : "/." */
                        	id --;
                        is ++;
                        continue;
                }

                /* XXX/.. seen */			
                if(s[is] == '.' && s[is + 1] == '.' && s[is + 2] == '\0'){
                        is += 2;
                        if(id == 0 || id == 1){
                                /* Out of root range */
                                s[0] = 0;
                                return -1;
                        }

                        id --;
                        while(id > 0 && s[--id] != '/');
                        if(id == 0){
                                s[0] = 0;
                                return -1;
                        }
                        continue;
                }

                /* XXX/../ seen */			
                if((s[is] == '.') && (s[is + 1] == '.')
                         &&(s[is + 2] == '/' || s[is + 2] == '\\')){
                        slashseen = 1;
                        is += 3;
                        if(id == 0 || id == 1){
                                /* Out of root range */
                                s[0] = 0;
                                return -1;
                        }
                        id--;
                        while(id > 0 && s[--id] != '/') ;

                        if(s[id] == '/'){
                                id++;
			}else{
                                s[0] = 0;
                                return -1;
                        }
                        continue;
                }

                while(is < ls){
                        s[id] = s[is];
                        if(s[is] == '/' || s[is] == '\\')
                                break;
                        id ++;
                        is ++;
                }
        }

        return 0;
}


/********************************************

  Function:	character_to_digit

  Description:	Convert character to digit. 

  Return:	-1	fault
		>= 0	success  

********************************************/ 	
  
static int32_t _http_character_to_digit(u_int8_t ch)
{
        u_int8_t tmp;
        int32_t digit = -1;
        
        tmp = ch;
        tmp |= 0x20;    //convert 'A'-'F' -> 'a'-'f'                                                
             
        if((tmp >= 'a') && (tmp <= 'f')){ 
	        digit = tmp - 'a' + 0xa;
        }       
        else if((tmp >= '0') && (tmp <= '9')){ 
        	digit = tmp - '0';
        }else{
		return -1;
	}       
                
        return digit;
}

/*********************************************

  Function:	percent_xx_decode_backward

  Description:	Convert "%xx" to digit. 

  Return:	==0	no string converted
		> 0	success  

*********************************************/ 	

static int32_t _http_percent_xx_decode_backward(u_int8_t *string, int32_t string_len)
{
        u_int8_t ch;
        int8_t digit1 = 0;
        int8_t digit2 = 0;
        int32_t decode_len = 0;
	
	if(string_len > 1){
                digit1 = _http_character_to_digit(string[string_len - 1]);
                digit2 = _http_character_to_digit(string[string_len - 2]);
		if((digit1 >= 0) && (digit2 >= 0)){
                        if( digit1 == 0 && digit2 == 0){
                                decode_len = 0;
                        }else{
                                ch = digit1;
                                ch <<= 4;
                                ch = ch + digit2;
                                string[string_len - 2] = ch;
                                decode_len = 1;
                        }
                }else{
                        decode_len = 0;
                }

        }
        return decode_len;
}

/*********************************************

  Function:	percent_FFxx_decode_backward

  Description:	Convert "%UFFxx" to digit. 

  Return:	==0	no string converted
		> 0	success  

*********************************************/ 	

static int32_t _http_percent_FFxx_decode_backward(u_int8_t *string, int32_t string_len)
{
        u_int8_t ch;
        int8_t digit1 = 0;
        int8_t digit2 = 0;
        int32_t decode_len = 0;
	
	if(string_len > 1){
                digit1 = (_http_character_to_digit(string[string_len - 1]) + 2);
                digit2 = _http_character_to_digit(string[string_len - 2]);
                //if((digit1 >= 0) && (digit2 >= 0)){
                        ch = digit1;
                        ch <<= 4;
                        ch = ch + digit2;
                        string[string_len - 2] = ch;
                        decode_len = 1;
                //}else{
		//	decode_len = 0;
		//}
        }
        return decode_len;
}



/*****************************************************************************

  Function:	percent_combine

  Description:	Combine '%',as "%2525..","%X25X25..","%U0025U0025..",etc. 

  Return:	== 0	no percent combined
		> 0	the combine_len's string combined 

*****************************************************************************/ 	

static int32_t _http_percent_combine(u_int8_t *string, int32_t *string_len)
{
	int32_t combine_len = 0;

	if((*string_len >= 6)&&!strncasecmp("5200u%",(char *)&string[*string_len - 6],6)){
		string[*string_len - 6] = '%';
		combine_len = 5;
		goto ret;
	}
	
	if((*string_len >= 4)&&!strncasecmp("52x%",(char *)&string[*string_len - 4],4)){
		string[*string_len - 4] = '%';
		combine_len = 3;
		goto ret;
	}

	if((*string_len >= 3)&&!strncasecmp("52%",(char *)&string[*string_len - 3],3)){
		string[*string_len - 3] = '%';
		combine_len = 2;
		goto ret;
	}

ret:
	*string_len -= combine_len;	
	if(*string_len >= 0){
		return combine_len;
	}else{
		return 0;
	}
}

/*********************************************************

  Function:	string_decode_type_backward

  Description:	Get the encoding type and erase the '%' 

  Return:	-1	fault
		>= 0	success

*********************************************************/ 	

static int32_t _http_string_decode_type_backward(u_int8_t *string, int32_t *string_len,int tmp_len)
{
	int32_t string_type = -1;
	int32_t digit1 = -1, digit2 = -1;
	int32_t combine_len = 0;
	
	//Step1: combine '%', as "%2525..","%X25X25..","%U0025U0025..",etc.
	if( tmp_len == -1){
		combine_len = _http_percent_combine(string,string_len);
	
		while(combine_len >0){
			combine_len = _http_percent_combine(string,string_len);
		}
	}
	
	if(*string_len <= 2) {
		string_type = NO_PERCENT;
		goto ret;//xrg
	}
	
	//Step2: get the encoding type as %X41
	if(((string[*string_len - 2] == 'x')||(string[*string_len - 2] == 'X')) && (tmp_len>=3 || tmp_len == -1)){ 
		if(*string_len <= 3){
			string_type = NO_PERCENT;
			goto ret;
		}
		digit1 = _http_character_to_digit(string[*string_len - 3]);
		digit2 = _http_character_to_digit(string[*string_len - 4]);

		//if((digit1 >= 0) && (digit1 <= 7) && (digit2 >= 0)){
			string_type = PERCENT_XYY;
			goto ret;
		//}else{
		//	string_type = IGNORANCE_XYY;
		//	goto ret;
		//}
	}

	//Step3: get the encoding type as %U0041
	if(!strncasecmp("00u%",(char *)&string[*string_len - 4],4) && (tmp_len>=5 || tmp_len == -1)){ 
		if(*string_len <= 5){
			string_type = NO_PERCENT;
			goto ret;
		}
		digit1 = _http_character_to_digit(string[*string_len - 5]);
		digit2 = _http_character_to_digit(string[*string_len - 6]);
		//if((digit1 >= 0) && (digit1 <= 7) && (digit2 >= 0)){
			string_type = PERCENT_U00XX;
			goto ret;
		//}else{
		//	string_type = IGNORANCE_U00XX;
		//	goto ret;
		//}
	}

	if(!strncasecmp("ffu%",(char *)&string[*string_len - 4],4) && (tmp_len>=5 || tmp_len == -1)){ 
		if(*string_len <= 5){
			string_type = NO_PERCENT;
			goto ret;
		}

		string_type = PERCENT_UFFXX;
		goto ret;
	}

	//Step4: get the encoding type as %41
	digit1 = _http_character_to_digit(string[*string_len - 2]);
	digit2 = _http_character_to_digit(string[*string_len - 3]);

//	if((digit1 >= 0) && (digit1 <= 7) && (digit2 >= 0)){
		if(  (tmp_len>=2 || tmp_len == -1)){
			string_type = PERCENT_XX;
			goto ret;
		}
//	}else{
//		string_type = IGNORANCE_XX;
//		goto ret;
//	}
ret:
	return string_type;
}

/*********************************************************

  Function:	string_decode_backward

  Description:	Decoding the strings from end to begin. 

  Return:	-1	fault
		>= 0	success

*********************************************************/ 	

static int32_t _http_string_decode_backward(u_int8_t *out_string, int32_t *out_len,int tmp_len)
{
	int32_t decode_len = 0;
	int32_t string_type = -1;

	//Step1: get the encoding type and erase the '%'
	string_type = _http_string_decode_type_backward(out_string,out_len,tmp_len);
	if(string_type < 0){
		return *out_len;
	}

	//Step2: decode the string according to it's type
	switch(string_type){
		case PERCENT_XX:	//format as %41
			*out_len -= 1;
			decode_len = _http_percent_xx_decode_backward(out_string,*out_len);
			if(decode_len){
				*out_len -= decode_len;
			}else{
				*out_len += 1;
			}
			break;
		case PERCENT_U00XX:	//format as %U0041
			*out_len -= 4;
			decode_len = _http_percent_xx_decode_backward(out_string,*out_len);
			if(decode_len){
				*out_len -= decode_len;
			}else{
				*out_len += 4;
			}
			break;
		case PERCENT_UFFXX:	//format as %UFF41
			*out_len -= 4;
			decode_len = _http_percent_FFxx_decode_backward(out_string,*out_len);
			if(decode_len){
				*out_len -= decode_len;
			}else{
				*out_len += 4;
			}
			break;
		case PERCENT_XYY:
			*out_len -= 2;	//format as %X41
			decode_len = _http_percent_xx_decode_backward(out_string,*out_len);
			if(decode_len){
				*out_len -= decode_len;
			}else{
				*out_len += 2;
			}
			break;
		case IGNORANCE_XX:	//format as %ff, to be ignored.
//			*out_len -= 3;
			break;
		case IGNORANCE_U00XX:	//format as %U00ff, to be ignored.
//			*out_len -= 6;
			break;
		case IGNORANCE_XYY:	//format as %Xff, to be ignored.
//			*out_len -= 4;
			break;
		default:
			break;
	}
	return *out_len;
}

/*****************************************************************************

  Function:	string_decode

  Description:	Decoding the strings encoded witch the format of url-evasion. 

  e.g:		"%41","%2541","%x41","%%34%31","%%341","%4%31","%U0041",
		"%U0025%550%303%37",etc.

  Return:	out_len(The length of strings after decoded)
		== 0	fault
		>  0	success
		%2%2%25252541

*****************************************************************************/ 	
int32_t _http_string_decode(u_int8_t *out_string, u_int8_t *in_string, int32_t in_string_len,int decode_flag)
{
	int32_t i = 0;
	int32_t out_len = 0;
	u_int8_t tmp_char = 0;
	int tmp_len=0;
	if( out_string == NULL || in_string == NULL || in_string_len <=0){
		return 0;
	}

	for(i = in_string_len - 1; i >= 0; i--){	//decode the string from end to begin.
		tmp_char = in_string[i];
		if(tmp_char == '%'){	//find the '%', and decode the string from begin to '%'.
			out_string[out_len] = tmp_char;
			out_len++;
			if( decode_flag == ONLY_ONE_DECODE){	
				if(_http_string_decode_backward(out_string,&out_len,tmp_len) < 0){
					return -1;
				}
			}else{
				if(_http_string_decode_backward(out_string,&out_len,-1) < 0){
					return -1;
				}
			}
			if( decode_flag == ONLY_ONE_DECODE){	
				tmp_len=0;
			}
		}else if(tmp_char == '+'){	//find the '+', and replace to SPACE.
			out_string[out_len] = ' ';
			out_len++;
		}else{
			out_string[out_len] = tmp_char;
			out_len++;
			if( decode_flag == ONLY_ONE_DECODE){	
				tmp_len++;
			}
		}
	}
	return out_len;
}

/***********************************************************************************

  Function:	reverse_string

  Description:	Reverse the strings character by character, don't change the length. 

  Return:	NULL		

***********************************************************************************/ 	
static void _http_reverse_string(u_int8_t *string, int32_t string_len)
{
	u_int8_t ch_tmp = 0;
	int32_t i = 0;
	if( string == NULL || string_len <=0){
		return ;
	}
	for(i = 0;i <= string_len/2 - 1;i++){
		ch_tmp = string[i];
		string[i] = string[string_len - i - 1];
		string[string_len - i - 1] = ch_tmp;
	}
}

#if 0
int http_get_url_core(unsigned char **url_out, unsigned char *url_in, int url_in_len)
{
	int outi = 0;

	if (url_out == NULL || url_in == NULL || url_in_len <= 0 ) {
		outi = -1;
		goto ret;
	}

	while (url_in_len > 0 && (*url_in == ' ' || *url_in ==0x09)) {
		url_in ++;
		url_in_len --;
	}

	while (url_in_len > 0 && ( url_in[url_in_len - 1] == ' ' || url_in[url_in_len - 1] == 0x09)) {
		url_in_len --;
	}

	if (url_in_len > 0) {
		//outi = strlcpy2((char*)url_out, max_url_out_len, (char*)url_in, url_in_len);
		outi=url_in_len;
		*url_out=url_in;
	}

ret:
	return outi;
}
#endif

static int _http_html_entities_decode_inplace(unsigned char *input, int input_len) ;
static int _http_escape_decode_inplace(unsigned char *input, int input_len) ;
/*static int _http_check_need_decode(unsigned char* str,int len)
{
	int i=0;
	for( i=0;i<len;i++){
		if( str[i]=='%' || str[i]=='&' || str[i]=='\\' || str[i]=='+'){
			return 1;
		}
	}
	return 0;
}*/

int http_decode_string(unsigned char *out_string, \
        int max_url_out_len, unsigned char *in_string, int in_string_len,int data_flag)
{
        //if( check_need_decode(in_string,in_string_len) == 0){
         //       return -1;
        //}
        int out_string_len = 0;
        char tmp_buf[MAX_DECODE_LEN]={0};
        int tmp_buf_len=0;
        int new_len1=0;
        int new_len2=0;
        int new_len3=0;


        if( in_string_len > max_url_out_len){
                goto ret;
        }
        if ((NULL == in_string) || (in_string_len <= 0)) {
                goto ret;
        }

	if( in_string_len> MAX_DECODE_LEN){
		return -1;
	}
        memcpy(tmp_buf, (char*)in_string, in_string_len);
	tmp_buf[in_string_len]='\0';
	tmp_buf_len=in_string_len;
        int flag=1;

        int cnt=16;
        while( flag ==1 && cnt-->0){
                flag=0;
                if(GET_NEED_HTML_DECODE(data_flag) !=0){
                        new_len1=_http_html_entities_decode_inplace((unsigned char*)tmp_buf,tmp_buf_len);
                        tmp_buf[new_len1]='\0';
                        if( new_len1 != tmp_buf_len){
                        	tmp_buf_len=new_len1;
                                flag =1;
                        }
                }


                if(GET_NEED_ESCAPE_DECODE(data_flag) !=0){
                        new_len2=_http_escape_decode_inplace((unsigned char*)tmp_buf, tmp_buf_len) ;
                        tmp_buf[new_len2]='\0';
                        if( tmp_buf_len != new_len2){
                        	tmp_buf_len=new_len2;
                                flag =1;
                        }
                }

                if(GET_NEED_URL_DECODE(data_flag) !=0){
                        if(in_string_len > 2){
                                new_len3 = _http_string_decode(out_string,(unsigned char*)tmp_buf,tmp_buf_len,0);
                                _http_reverse_string(out_string, new_len3);
                        }else{
                                new_len3 = strlcpy2((char*)out_string, max_url_out_len, (char*)tmp_buf, tmp_buf_len);
                        }
                        out_string[new_len3]='\0';
                        //if( new_len3 != tmp_buf_len){
                                flag =1;
				tmp_buf_len = new_len3;
                                tmp_buf_len = strlcpy2(tmp_buf, sizeof(tmp_buf), (char*)out_string, new_len3);
                        //}
                }

		
		out_string_len = strlcpy2((char*)out_string, max_url_out_len, (char*)tmp_buf, tmp_buf_len);

        }




ret:
        return out_string_len;
}

/**
 *
 * IMP1 Assumes NUL-terminated
 */
static int _http_html_entities_decode_inplace(unsigned char *input, int input_len) {
    unsigned char *d = input;
    int i, count;
    char *x = NULL;

    if ((input == NULL)||(input_len <= 0) || input_len >MAX_DECODE_LEN) return 0;

    i = count = 0;
    while((i < input_len)&&(count < input_len)) {
        int z, copy = 1;

        /* Require an ampersand and at least one character to
         * start looking into the entity.
         */
        if ((input[i] == '&')&&(i + 1 < input_len)) {
            int k, j = i + 1;

            if (input[j] == '#') {
                /* Numerical entity. */
                copy++;

                if (!(j + 1 < input_len)) goto HTML_ENT_OUT; /* Not enough bytes. */
                j++;

                if ((input[j] == 'x')||(input[j] == 'X')) {
                    /* Hexadecimal entity. */
                    copy++;

                    if (!(j + 1 < input_len)) goto HTML_ENT_OUT; /* Not enough bytes. */
                    j++; /* j is the position of the first digit now. */

                    k = j;
                    while((j < input_len)&&(isxdigit(input[j]))) j++;
                    if (j > k) { /* Do we have at least one digit? */
                        /* Decode the entity. */
			x = (char *)&input[k];
                        *d++ = (unsigned char)strtol(x, NULL, 16);
                        count++;

                        /* Skip over the semicolon if it's there. */
                        if ((j < input_len)&&(input[j] == ';')) i = j + 1;
                        else i = j;

                        continue;
                    } else {
                        goto HTML_ENT_OUT;
                    }
                } else {
                    /* Decimal entity. */
                    k = j;
                    while((j < input_len)&&(isdigit(input[j]))) j++;
                    if (j > k) { /* Do we have at least one digit? */
                        /* Decode the entity. */
			x = (char *)&input[k];
                        *d++ = (unsigned char)strtol(x, NULL, 10);
                        count++;

                        /* Skip over the semicolon if it's there. */
                        if ((j < input_len)&&(input[j] == ';')) i = j + 1;
                        else i = j;

                        continue;
                    } else {
                        goto HTML_ENT_OUT;
                    }
                }
            } else {
                /* Text entity. */

                k = j;
                while((j < input_len)&&(isalnum(input[j]))) j++;
                if (j > k) { /* Do we have at least one digit? */
		    x = (char *)&input[k];	

                    /* Decode the entity. */
                    /* ENH What about others? */
                    if (strncasecmp(x, "quot", 4) == 0) *d++ = '"';
                    else
                    if (strncasecmp(x, "amp", 3) == 0) *d++ = '&';
                    else
                    if (strncasecmp(x, "lt", 2) == 0) *d++ = '<';
                    else
                    if (strncasecmp(x, "gt", 2) == 0) *d++ = '>';
                    else
                    if (strncasecmp(x, "nbsp", 4) == 0) *d++ = NBSP;
                    else {
                        /* We do no want to convert this entity, copy the raw data over. */
                        copy = j - k + 1;
                        goto HTML_ENT_OUT;
                    }

                    count++;

                    /* Skip over the semicolon if it's there. */
                    if ((j < input_len)&&(input[j] == ';')) i = j + 1;
                    else i = j;

                    continue;
                }
            }
        }

        HTML_ENT_OUT:

        for(z = 0; ((z < copy) && (count < input_len)); z++) {
            *d++ = input[i++];
            count++;
        }
    }

    *d = '\0';

    return count;
}

static unsigned char x2c(unsigned char *what) {
    register unsigned char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));

    return digit;
}
#define ISODIGIT(X) ((X >= '0')&&(X <= '7'))
/* replaceComments */
static int _http_escape_decode_inplace(unsigned char *input, int input_len) {
    unsigned char *d = input;
    int i, count;

    i = count = 0;
    while(i < input_len) {
        if ((input[i] == '\\')&&(i + 1 < input_len)) {
            int c = -1;

            switch(input[i + 1]) {
                case 'a' :
                    c = '\a';
                    break;
                case 'b' :
                    c = '\b';
                    break;
                case 'f' :
                    c = '\f';
                    break;
                case 'n' :
                    c = '\n';
                    break;
                case 'r' :
                    c = '\r';
                    break;
                case 't' :
                    c = '\t';
                    break;
                case 'v' :
                    c = '\v';
                    break;
                case '\\' :
                    c = '\\';
                    break;
                case '?' :
                    c = '?';
                    break;
                case '\'' :
                    c = '\'';
                    break;
                case '"' :
                    c = '"';
                    break;
            }

            if (c != -1) i += 2;

            /* Hexadecimal or octal? */
            if (c == -1) {
                if ((input[i + 1] == 'x')||(input[i + 1] == 'X')) {
                    /* Hexadecimal. */
                    if ((i + 3 < input_len)&&(isxdigit(input[i + 2]))&&(isxdigit(input[i + 3]))) {
                        /* Two digits. */
                        c = x2c(&input[i + 2]);
                        i += 4;
                    } else {
                        /* Invalid encoding, do nothing. */
                    }
                }
                else
                if (ISODIGIT(input[i + 1])) { /* Octal. */
                    char buf[4];
                    int j = 0;

                    while((i + 1 + j < input_len)&&(j < 3)) {
                        buf[j] = input[i + 1 + j];
                        j++;
                        if (!ISODIGIT(input[i + 1 + j])) break;
                    }
                    buf[j] = '\0';

                    if (j > 0) {
                        c = strtol(buf, NULL, 8);
			printf(" %c\n",c);
                        i += 1 + j;
                    }
                }
            }

            if (c == -1) {
                /* Didn't recognise encoding, copy raw bytes. */
                *d++ = input[i ];
                count++;
                *d++ = input[i + 1];
                count++;
                i += 2;
            } else {
                /* Converted the encoding. */
                *d++ = c;
                count++;
            }
        } else {
            /* Input character not a backslash, copy it. */
            *d++ = input[i++];
            count++;
        }
    }


    return count;
}


#if 0
int http_msre_fn_replace_comments_execute(unsigned char *input,
    long int input_len)
{
    long int i, j, incomment;
    int changed = 0;

    i = 0;
    j = 0;
    incomment = 0;

    while(i < input_len) {
        if (incomment == 0) {
            if ((input[i] == '/')&&(i + 1 < input_len)&&(input[i + 1] == '*')) {
                changed = 1;
                incomment = 1;
                i += 2;
            } else {
                input[j] = input[i];
                i++;
                j++;
            }
        } else {
            if ((input[i] == '*')&&(i + 1 < input_len)&&(input[i + 1] == '/')) {
                incomment = 0;
                i += 2;
                input[j] = ' ';
                j++;
            } else {
                i++;
            }
        }
    }

    input[j] = '\0';

    return j;
}

/* lowercase */

int http_msre_fn_lowercase_execute(unsigned char *input,
    long int input_len)
{
    long int i;
    int changed = 0;

    i = 0;
    while(i < input_len) {
        int x = input[i];
        input[i] = tolower(x);
        if (x != input[i]) changed = 1;
        i++;
    }

    return input_len;
}
#endif

int32_t 
main_test_url(int32_t argc, int8_t *argv[])
//main(int32_t argc, char *argv[])
{
	if (argc != 2){
		printf("usage: test <string>\n");
		return -1;
	}

	if (argv[1]){
		printf("in_string is:\"%s\"\n",argv[1]);
	}else{
		printf("Input error!\n");
		return -1;
	}

	u_int8_t *out_string = NULL, *in_string = NULL;
	int32_t in_string_len = 0, out_string_len = 0;

	in_string = (unsigned char *)argv[1];
	in_string_len = strlen((char *)in_string);
	printf("in_string_len is:%d\n",in_string_len);
	out_string = malloc(MAX_STRING_LEN);
	if(!out_string){
		return -1;
	}

	if(in_string_len > 2){
		out_string_len = _http_string_decode(out_string,in_string,in_string_len,1);
		_http_reverse_string(out_string, out_string_len);
	}else{
		out_string_len = strlcpy2((char*)out_string, MAX_STRING_LEN, (char*)in_string, in_string_len);
	}

//	reverse_string(out_string,out_string_len);

	out_string[out_string_len]='\0';
	//_collapse_path(out_string,out_string_len); 
	printf("out_string is :\n");
	_http_print_buf2(out_string,out_string_len + 1, 0);
	http_decode_path(out_string, out_string_len); 
	out_string_len = strlen((char *)out_string);
	
	if(out_string_len > 0){
		printf("out_string_len is %d\n",out_string_len);
		_http_print_buf2(out_string, out_string_len, 0);
	}
	out_string_len = _http_html_entities_decode_inplace(out_string, out_string_len);
	printf("html_entities_decode_inplace(%d)is:\n", out_string_len);
	_http_print_buf2(out_string, out_string_len, 0);

#if 0
	out_string_len = http_msre_fn_replace_comments_execute(out_string, out_string_len);
	printf("msre_fn_replaceComments_execute(%d)is:\n", out_string_len);
	print_buf2(out_string, out_string_len, 0);

	out_string_len = http_msre_fn_lowercase_execute(out_string, out_string_len);
#endif
	return 0;	
}


