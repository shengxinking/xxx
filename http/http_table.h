#ifndef _HTTP_TABLE_H
#define _HTTP_TABLE_H
typedef int (*http_func_t)(char *data, int dlen,int off, http_t* info);

typedef struct _http_state
{
        int             state;
        http_func_t     func;
} http_state_t;

typedef struct _http_table__
{
        char            key[128];
        int             keylen;
        int             value;
        http_func_t     func;
} http_table_t;

typedef struct _http_hstate
{
        int             state;
} http_header_state_t;

enum{
        HTTP_HEADER_STATE_CHAR0,      //before start
        HTTP_HEADER_STATE_CHAR1,      //start
        HTTP_HEADER_STATE_CHARN,      //going on
        HTTP_HEADER_STATE_R1,         ///r
        HTTP_HEADER_STATE_N1,         ///r/n
        HTTP_HEADER_STATE_R2,         ///r/n/r
        HTTP_HEADER_STATE_N2,         ///r/n/r/n
        HTTP_HEADER_STATE_NUM,
};


int http_method_null(char* data,int len,int offset,http_t* info);

int http_init_mstate(void);
int http_init_header_table();
int http_init_method_table(void);
int http_init_header_state();



extern http_state_t            http_state[HTTP_STATE_NUM];
extern http_table_t          http_header[27][27][27][5];
extern http_header_state_t           http_header_state[HTTP_HEADER_STATE_NUM][3];
#define MAX_METHOD_TABLE_MIX 5
extern http_table_t          http_methods[27][27][MAX_METHOD_TABLE_MIX];


#endif
