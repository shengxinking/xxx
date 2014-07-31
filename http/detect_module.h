
#ifndef __DETECT_MODULE_H
#define __DETECT_MODULE_H

#include "load_rule.h"
#include "http.h"
#include "connection.h"

typedef struct module_detect_t{
	int ( *detect_entry )(http_t *p_http, connection_t *conn);
}module_detect_t;

extern int do_http_parse_test(http_t *p_http, char *buf , int len);
extern module_detect_t module_detect[MD_COUNT];
extern int load_waf_module();
extern int	module_detect_entry(http_t *p_http,connection_t *conn);
#endif
