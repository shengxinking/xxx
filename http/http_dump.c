#include "http.h"
#include "http_basic.h"
#include "http_api.h"
int http_dump_str(http_t* info)
{
	printf("****************** str begin***********************\n");
	printf(" raw url %d %s\n", http_get_str_len(info,HTTP_STR_RAW_URL),http_get_str(info,HTTP_STR_RAW_URL));
	printf(" url+arg  %d %s\n", http_get_str_len(info,HTTP_STR_URL_AND_ARG),http_get_str(info,HTTP_STR_URL_AND_ARG));
	printf(" url %d %s\n", http_get_str_len(info,HTTP_STR_URL),http_get_str(info,HTTP_STR_URL));
	printf(" agent %d %s\n", http_get_str_len(info,HTTP_STR_AGENT),http_get_str(info,HTTP_STR_AGENT));
	printf(" auth %d %s\n", http_get_str_len(info,HTTP_STR_AUTH),http_get_str(info,HTTP_STR_AUTH));
	printf(" user %d %s\n", http_get_str_len(info,HTTP_STR_USER),http_get_str(info,HTTP_STR_USER));
	printf(" passwd %d %s\n", http_get_str_len(info,HTTP_STR_PASSWD),http_get_str(info,HTTP_STR_PASSWD));
	printf(" refer %d %s\n", http_get_str_len(info,HTTP_STR_REFER),http_get_str(info,HTTP_STR_REFER));
	printf(" location %d %s\n", http_get_str_len(info,HTTP_STR_LOCATION),http_get_str(info,HTTP_STR_LOCATION));
	printf(" content type %d %s\n", http_get_str_len(info,HTTP_STR_CONTENT_TYPE),http_get_str(info,HTTP_STR_CONTENT_TYPE));
	printf(" host %d %s\n", http_get_str_len(info,HTTP_STR_HOST),http_get_str(info,HTTP_STR_HOST));
	printf(" retcode %d %s\n", http_get_str_len(info,HTTP_STR_RETCODE),http_get_str(info,HTTP_STR_RETCODE));
	printf("****************** str end***********************\n\n\n");
	return 0;
}
int http_dump_int(http_t* info)
{
	printf("****************** int begin***********************\n");
	printf(" ret code %d\n", http_get_int(info,HTTP_INT_RET_CODE));
	printf(" header len %d\n", http_get_int(info,HTTP_INT_HEADER_LEN));
	printf(" content len %d\n", http_get_int(info,HTTP_INT_CONTENT_LEN));
	printf(" max header line len%d\n", http_get_int(info,HTTP_INT_MAX_HEADER_LINE_LEN));
	printf(" body len %d\n", http_get_int(info,HTTP_INT_BODY_LEN));
	printf(" url arg len %d\n", http_get_int(info,HTTP_INT_URL_ARG_LEN));
	printf(" url arg cnt %d\n", http_get_int(info,HTTP_INT_URL_ARG_CNT));
	printf("****************** int end***********************\n\n\n");
	return 0;
}
int http_dump_file(http_t* info)
{
        int i=0;
        int cnt=http_get_file_cnt(info);
        http_file_t* file=NULL;
        for (i = 0; i < cnt; i++) {
                file = http_get_file(info,i);
                printf("name[%s], value [%s]\n", file->name.p, file->value.p);
        }
        return 0;

}


int http_dump_logic(http_t* info)
{
	printf("****************** logic begin***********************\n");
	printf(" method %d\n", http_get_logic(info,HTTP_LOGIC_METHOD));
	printf(" close %d\n", http_get_logic(info,HTTP_LOGIC_IS_CLOSED));
	printf(" content type %d\n", http_get_logic(info,HTTP_LOGIC_CONTENT_TYPE));
	printf(" state %d\n", http_get_logic(info,HTTP_LOGIC_STATE));
	printf(" chunked %d\n", http_get_logic(info,HTTP_LOGIC_IS_CHUNKED));
	printf("****************** logic end***********************\n\n\n\n");
	return 0;
}
int http_dump_arg(http_t* info)
{
	http_arg_ctrl_t *ctl=http_get_arg_t(info);
	http_arg_t *arg=NULL;
	int num=ctl->num;
	int i=0;
	printf("****************** arg begin***********************\n");
	for( i=0;i<num;i++){
		arg=&ctl->arg[i];
		printf(" %s:%s\n",arg->name.p,arg->value.p);
	}
	printf("****************** end begin***********************\n");
	return 0;
}
