#include "proxy.h"
#include "thread_fun.h"
#include <unistd.h>
#include "log_db.h"


unsigned long long g_clock = 0;


static int _update_traffic(thread_t *info)
{
		tlog_t tlog;
		
        proxy_stat_lock();
        tlog.clirecv = g_cfg.stat.nclirecv;
        tlog.svrrecv = g_cfg.stat.nsvrrecv;
        tlog.nlive = g_cfg.stat.nlive;
        tlog.nowlive = g_cfg.stat.nowlive;
		proxy_log_lock();
		log_traffic(&tlog);
		proxy_log_unlock();
		g_cfg.stat.nsvrrecv = 0;
        g_cfg.stat.nclirecv = 0;
		g_cfg.stat.nlive = 0;
        proxy_stat_unlock();

        return 0;
}



static int _clock_verify(thread_t *info)
{
        if (!info) {
                printf("clock thread invalid param\n");
                return -1;
        }

        if (info->type != THREAD_CLOCK) {
                printf("clock thread invalid type\n");
                return -1;
        }

        return 0;
}

void *clock_run(void *arg)
{
        thread_t *info;

        info = arg;

        if (_clock_verify(info))
                pthread_exit(0);

        while (!g_cfg.stop) {
                sleep(300);
                g_clock++;
                _update_traffic(info);
        }

       printf("clock thread is exited\n");

        return NULL;
}
