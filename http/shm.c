#include <stdio.h>
#include <malloc.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "queue.h"
#include "log_db.h"
#include "proxy.h"

#define SHM_ID_A 0x1234
#define SHM_ID_T 0x6789

void*  Create_shm(int size, int shmid)
{
	void *mem = NULL;
	int id ;

	id = shmget(shmid ,size, 0666|IPC_CREAT );
	mem = shmat(id, ( const void* )0, 0 );
    if (mem == (void*)-1) {
		printf("shmat error \n");
		return NULL;
	}
	return mem;
}

int init_shm_queue(void *mem, int max, int size)
{
	void *p ;
	int i; 
	queue_t *q	= mem;

	q->front = 0;       
	q->rear = 0;
	q->objsize = size;
	q->maxsize = max;

	q->pBase = mem + sizeof(queue_t);
	p = q->pBase + sizeof(char*)*max;
	
	for (i = 0; i < max; i++) {
		q->pBase[i] = (void*)p + i*(size);
	}
	
	return 0;
}


int init_alog_mem(int shmid, int max)
{
	void *mem = NULL;
	int size = sizeof(queue_t) + sizeof(char*)*max + sizeof(alog_t)*max;
	mem = Create_shm(size,shmid);
	init_shm_queue(mem, max, sizeof(alog_t));

	g_cfg.slog.shm_a = mem;
	return 0;
}



int init_tlog_mem(int shmid, int max)
{
	void *mem = NULL;
	int size = sizeof(queue_t) + sizeof(char*)*max + sizeof(tlog_t)*max;
	mem = Create_shm(size, shmid);
	init_shm_queue(mem, max, sizeof(tlog_t));
	
	g_cfg.slog.shm_t = mem;
	return 0;
}


int init_log_mem(int max)
{
	init_tlog_mem(SHM_ID_A, max);
	init_alog_mem(SHM_ID_T, max);

	return 0;
}

