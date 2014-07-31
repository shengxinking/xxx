#include "http.h"
#include "http_table.h"
#include "http_basic.h"

http_state_t            http_state[HTTP_STATE_NUM];
http_table_t          http_header[27][27][27][5];
http_header_state_t           http_header_state[HTTP_HEADER_STATE_NUM][3];

http_table_t          http_methods[27][27][MAX_METHOD_TABLE_MIX];


int set_method_table(char* method,int method_len,int value,int num)
{
	if( method==NULL){
		return 0;
	}
	if( method_len <2){
		return 0;
	}
	http_table_t* table=&http_methods[http_char2num(method[0])][http_char2num(method[1])][num];
	strcpy(table->key,method);
	table->keylen=method_len;
	//table->func=func;
	table->value=value;
	return 0;
	
}
int http_method_null(char* data,int len,int offset,http_t* info)
{

        return 0;
}


int 
http_init_method_table(void)
{
	int i=0;
	int j=0;
	int m=0;
	
	for(i=0;i<27;i++){
		for(j=0;j<27;j++){
			for(m=0;m<MAX_METHOD_TABLE_MIX;m++){
				http_methods[i][j][m].func=http_method_null;
				http_methods[i][j][m].keylen=0;
			}
		}
	}

	char* method=NULL;

	method="connect";
	set_method_table(method,strlen(method),HTTP_METHOD_CONNECT,0);
	method="copy";
	set_method_table(method,strlen(method),HTTP_METHOD_WEBDAV,1);
	
	method="DELETE";
	set_method_table(method,strlen(method),HTTP_METHOD_DELETE,0);

	method="head";
	set_method_table(method,strlen(method),HTTP_METHOD_HEAD,0);

	method="get";
	set_method_table(method,strlen(method),HTTP_METHOD_GET,0);

	method="lock";
	set_method_table(method,strlen(method),HTTP_METHOD_WEBDAV,0);

	method="mkcol";
	set_method_table(method,strlen(method),HTTP_METHOD_WEBDAV,0);

	method="move";
	set_method_table(method,strlen(method),HTTP_METHOD_WEBDAV,0);

	method="OPTIONS";
	set_method_table(method,strlen(method),HTTP_METHOD_OPTION,0);

	method="post";
	set_method_table(method,strlen(method),HTTP_METHOD_POST,0);

	method="put";
	set_method_table(method,strlen(method),HTTP_METHOD_PUT,0);

	method="PROPFIND";
	set_method_table(method,strlen(method),HTTP_METHOD_WEBDAV,0);
	method="PROPPATCH";
	set_method_table(method,strlen(method),HTTP_METHOD_WEBDAV,1);

	method="TRACE";
	set_method_table(method,strlen(method),HTTP_METHOD_TRACE,0);

	method="UNLOCK";
	set_method_table(method,strlen(method),HTTP_METHOD_WEBDAV,0);

	return 0;
}
int http_init_mstate(void)
{
        /*http_state[HTTP_STATE_INIT].func=http_parse_head;
        http_state[HTTP_STATE_HEAD].func=http_parse_head;
        http_state[HTTP_STATE_BODY].func=http_parse_body;
        http_state[HTTP_STATE_NONE].func=http_parse_head;
        http_state[HTTP_STATE_ERROR].func=http_parse_head;*/

        return 0;
}

int http_init_header_state(void)
{
        http_header_state[HTTP_HEADER_STATE_CHAR0][0].state=HTTP_HEADER_STATE_CHAR1;
        http_header_state[HTTP_HEADER_STATE_CHAR0][1].state=HTTP_HEADER_STATE_CHAR0;
        http_header_state[HTTP_HEADER_STATE_CHAR0][2].state=HTTP_HEADER_STATE_CHAR0;

        http_header_state[HTTP_HEADER_STATE_CHAR1][0].state=HTTP_HEADER_STATE_CHARN;
        http_header_state[HTTP_HEADER_STATE_CHAR1][1].state=HTTP_HEADER_STATE_R1;
        http_header_state[HTTP_HEADER_STATE_CHAR1][2].state=HTTP_HEADER_STATE_CHARN;

        http_header_state[HTTP_HEADER_STATE_CHARN][0].state=HTTP_HEADER_STATE_CHARN;
        http_header_state[HTTP_HEADER_STATE_CHARN][1].state=HTTP_HEADER_STATE_R1;
        http_header_state[HTTP_HEADER_STATE_CHARN][2].state=HTTP_HEADER_STATE_CHARN;

        http_header_state[HTTP_HEADER_STATE_R1][0].state=HTTP_HEADER_STATE_CHAR1;
        http_header_state[HTTP_HEADER_STATE_R1][1].state=HTTP_HEADER_STATE_R1;
        http_header_state[HTTP_HEADER_STATE_R1][2].state=HTTP_HEADER_STATE_N1;

        http_header_state[HTTP_HEADER_STATE_N1][0].state=HTTP_HEADER_STATE_CHAR1;
        http_header_state[HTTP_HEADER_STATE_N1][1].state=HTTP_HEADER_STATE_R2;
        http_header_state[HTTP_HEADER_STATE_N1][2].state=HTTP_HEADER_STATE_N1;

        http_header_state[HTTP_HEADER_STATE_R2][0].state=HTTP_HEADER_STATE_R2;
        http_header_state[HTTP_HEADER_STATE_R2][1].state=HTTP_HEADER_STATE_R2;
        http_header_state[HTTP_HEADER_STATE_R2][2].state=HTTP_HEADER_STATE_N2;

        http_header_state[HTTP_HEADER_STATE_N2][0].state=HTTP_HEADER_STATE_CHAR1;
        http_header_state[HTTP_HEADER_STATE_N2][1].state=HTTP_HEADER_STATE_N2;
        http_header_state[HTTP_HEADER_STATE_N2][2].state=HTTP_HEADER_STATE_N2;
        return 0;
}
                    

