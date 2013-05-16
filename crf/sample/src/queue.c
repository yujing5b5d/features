#include "queue.h"
#include <stdio.h> // NULL

void clearQueue(pCNSQ Q)
{ 
    Q->front=Q->rear=0;
}

// test if the queue is empty
int isEmptyQueue(pCNSQ Q)
{
    return Q->front==Q->rear ;
}

int isFullQueue(pCNSQ Q)
{
    return (Q->rear+1)%CNSQSIZE==Q->front;
}

int enQueue(pCNSQ Q,CrfNodeSnapshot e)
{
    if((Q->rear+1)%CNSQSIZE==Q->front){ // full
    	return 0;
    }else{ 
        Q->rear=(Q->rear+1)%CNSQSIZE;
        Q->data[Q->rear]=e;
        return 1;
    }
}

int enQueueWithDrop(pCNSQ Q,CrfNodeSnapshot e)
{
	if(isFullQueue(Q)) deQueue(Q);
	enQueue(Q,e);
	return 1;
}

pCrfNodeSnapshot deQueue(pCNSQ Q)
{    
    if(isEmptyQueue(Q)){ // empty
        return NULL;
    }else {
        Q->front=(Q->front+1)%CNSQSIZE;
        return &(Q->data[Q->front]);
    }
}

pCrfNodeSnapshot nextNElem(pCNSQ Q,int N)
{
    if((Q->front+N)%CNSQSIZE >=Q->rear || N >= CNSQSIZE || N < 1){ // no such
        return NULL;
    }else {
        return &(Q->data[(Q->front+N)%CNSQSIZE]);
    }	
}

pCrfNodeSnapshot pastNElem(pCNSQ Q,int N)
{
    if((Q->front+N)%CNSQSIZE >=Q->rear || N >= CNSQSIZE || N < 1){ // no such
        return NULL;
    }else {
        return &(Q->data[(Q->rear+CNSQSIZE-N+1)%CNSQSIZE]);
    }
}



int sizeQueue(pCNSQ Q)
{
    int i=(Q->rear-Q->front+CNSQSIZE)%CNSQSIZE;
    return i;
}


