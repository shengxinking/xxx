#ifndef __qUEUE_H_
#define __qUEUE_H_
#include <time.h>
#include <inttypes.h>
#include <pthread.h>

typedef struct _t_queue 
{
	void **pBase;
	int front;
	int rear;
	int maxsize;
	int objsize;
	pthread_mutex_t lock;
}queue_t;

void Createqueue(queue_t * q,int max, int size);
void Traversequeue(queue_t * q);
int Fullqueue(queue_t * q);
int Emptyqueue(queue_t * q);
int  Enqueue(queue_t * q, void *val);
void* Dequeue(queue_t * q);
#endif
