#ifndef _HTTP_POLL_H
#define _HTTP_POLL_H


typedef struct _http_arg{
        http_str_t      name;
        http_str_t      value;
        int complete_flag;
        int             arg_flag;
        int             name_flag;
        int             value_flag;
} http_arg_t;

typedef struct _http_argctrl{
        http_arg_t      *cur;
        short           num;
        short           new_num;
        short           max_num;
        int             extern_flag;
	http_arg_t 	arg[32];
}http_arg_ctrl_t;

http_arg_t* http_get_arg(http_t* info,int num);
int  http_get_arg_cnt(http_t* info);


#endif
