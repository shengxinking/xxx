

#include<stdio.h>
#include<stdlib.h>
#include"malloc.h"
#include"queue.h"
#include <string.h>
/***********************************************
 * Function: Create a empty stack;
 * ************************************************/
void Createqueue(queue_t * q,int max, int size)
{
	int i = 0;
	
	q->pBase = malloc(sizeof(char*)*max);
	if(NULL == (q->pBase))
	{
		printf("Memory allocation failure");
		exit(-1);    
	}  

	for (i = 0; i < max; i++) {
		q->pBase[i] = (void*)malloc(size);
		if(NULL == (q->pBase[i])) {
			printf("Memory allocation failure");
			exit(-1);    
		}
	//	printf("%x ",q->pBase[i]);
	}
	q->front=0;       
	q->rear=0;
	q->objsize = size;
	q->maxsize = max;
	

}
/***********************************************
 * Function: Print the stack element;
 * ************************************************/
void Traversequeue(queue_t * q)
{
	int i = q->front;
	while (i%q->maxsize != q->rear)
	{
	//	printf("%x ",q->pBase[i]);
		i++;
	}
	printf("\n");
}
	
int Fullqueue(queue_t * q)
{
	if(q->front == (q->rear+1)%q->maxsize)   
		return 1;
	else
		return 0;
}

int Emptyqueue(queue_t * q)
{
	if(q->front == q->rear)   
		return 1;
	else
		return 0;
}


static int _Enqueue(queue_t * q, void *val)
{
	if(Fullqueue(q)) {
		printf("queue is full !!!\n");
		return -1;
	}
	else
	{
		memcpy(q->pBase[q->rear],val,q->objsize);
		q->rear=(q->rear+1)%q->maxsize;
		return 0;
	}
}


int Enqueue(queue_t * q, void *val)
{
	int ret = 0;

	pthread_mutex_lock(&q->lock);
	ret = _Enqueue(q, val);
	pthread_mutex_unlock(&q->lock);

	return ret;
}




void* _Dequeue(queue_t * q)
{
	void *val = NULL;

	if(Emptyqueue(q))
	{
		return NULL;
	}
	else
	{
		val = q->pBase[q->front];
		q->front = (q->front+1)%q->maxsize;
		return val;
	}
}

void* Dequeue(queue_t * q)
{
	void *val = NULL;

	pthread_mutex_lock(&q->lock);
	val = _Dequeue(q);
	pthread_mutex_unlock(&q->lock);
	
	return val;
}
