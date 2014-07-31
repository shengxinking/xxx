

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "debug.h"
#include "proxy.h"

static ip_port_t	_g_svraddr;	/* server address */
static ip_port_t	_g_rsaddr;/* real server address */
int		_g_wafid;	/* number of real server */

static void _usage(void)
{
	printf("tcpproxy <options> <(realsvr address(ip:port)>\n");
	printf("\t-a\tproxy address\n");
	printf("\t-w\twaf id\n");
	printf("\t-h\tshow help usage\n");
}


static int _parse_cmd(int argc, char **argv)
{
	return 0;
	char c;
	char optstr[] = ":a:w:r:h";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'a':
			if (ip_port_from_str(&_g_svraddr, optarg)) {
				printf("invalid proxy address: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'r':
			if (ip_port_from_str(&_g_rsaddr, optarg)) {
				printf("invalid real server address: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'w':
			_g_wafid = atoi(optarg);
			if (_g_wafid < 1 || _g_wafid >4) {
				printf("invalid work number: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("%c need argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option: %c\n", optopt);
			return -1;
		}
	}


	if (_g_svraddr.family == 0) {
		printf("no server address\n");
		return -1;
	}


	return 0;
}

int main(int argc, char **argv)
{
	if ( _parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}
	load_cfg();	

//	proxy_main2(&_g_svraddr, &_g_rsaddr);
	proxy_main2();

	return 0;
}




