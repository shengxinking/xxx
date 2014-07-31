#ifndef _HTTP_MODIFY_H_
#define _HTTP_MODIFY_H_
typedef struct http_modify_stream{
        char* new_data;
        int max_new_data_len;
        char* data;
        int data_len;

        int change_flag;
} http_modify_stream_t;

typedef struct http_header_modify_{
        char* url;
	int url_len;
	char* host;
	int host_len;
        char* auth;
	int auth_len;
        char* refer;
	int refer_len;
        char* xff;
	int xff_len;
        char* location;
	int location_len;
        char* content_len;
	int content_len_len;
} http_header_modify_t;



#endif
