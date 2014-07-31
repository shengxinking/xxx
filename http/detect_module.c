#include "detect_module.h"
#include "http_api.h"
#include "http_basic.h"

int  g_action = -1;
int  g_case = 0;

//log_other_t g_log_info;

module_detect_t module_detect[MD_COUNT];
char hint[MD_COUNT][64] ={
"rule check",
};


void load_rule_check()
{//6
	module_detect[MD_RULE_CHECK].detect_entry = rule_check_entry;
}



int load_waf_module()
{
	memset(module_detect, 0, MD_COUNT*sizeof(module_detect_t));
	load_rule_check();

	return 0;
}


int do_http_parse_test(http_t *p_http, char *buf , int len)
{
	local_t* local = http_get_local_t(p_http);
	int head_len = 0,ret = 0;
	if (local->state == 4){
		ret=http_parse_body(buf,len,p_http);
	}
	head_len = s_http_parse(buf,len,p_http);
	if (local->state < 4) {
		DBGH("state is < 4 ,header is not end\n");
		return -1;
	} else if (local->state == 4) {
		ret = http_parse_head(buf,head_len,0,p_http,0);
		ret = http_parse_body(buf+head_len,len-head_len,p_http);
	} else {
		DBGH("state = %d error \n", local->state);
	}
	
		
//	ret=http_parse_body(buf+head_len,1,p_http);
//        http_dump_logic(p_http);
//	http_dump_int(p_http);
//	http_dump_str(p_http);
	//http_dump_arg(p_http);

	return 0;
}

int	module_detect_entry(http_t *p_http,connection_t *conn)
{	
	int ret ,i = -1;
	module_detect_t *p_fun = NULL;

//	dump_policy(&config.policy);
	for(i = 1; i < MD_COUNT; i++) {
		
//		memset(&g_log_info, 0, sizeof(log_other_t));

		p_fun = &module_detect[i];
		if(!p_fun->detect_entry)
			continue;

		ret = p_fun->detect_entry(p_http, conn);
		if (ret == RET_PASS){
			return ret;
		}
		if (ret == RET_FAIL){
			DBGA("attack find in module [%s]\n", hint[i]);
			return ret;
		}
		//printf("module detect = %d\n", ret);
	}
	
	return RET_OK;
}
