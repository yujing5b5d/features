#ifndef __CRF_CTL_QUEUE_H__
#define __CRF_CTL_QUEUE_H__

#define CNSQSIZE 13

typedef struct
{
	int offset;
	int slen;
	char str[1024];
	int bf[30]; // basicfeature or basic flag
	int token;
}CrfNodeSnapshot,*pCrfNodeSnapshot;


typedef struct{
    CrfNodeSnapshot data[CNSQSIZE];  //队列的存储空间
    int front,rear;          //队头，队尾指针
}CNSQ,*pCNSQ; // CrfNodeSnapshotQueue


void clearQueue(pCNSQ Q);

// test if the queue is empty
int testQueue(pCNSQ Q);

int enQueue(pCNSQ Q,CrfNodeSnapshot e);

pCrfNodeSnapshot deQueue(pCNSQ Q);

pCrfNodeSnapshot nextNElem(pCNSQ Q,int N);

int sizeQueue(pCNSQ Q);


#endif // 
