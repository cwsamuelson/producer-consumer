#ifndef _QUEUE_A
#define _QUEUE_A

typedef struct {
   int *data;
   int tail, head, size, nelem;
} queue;

void init_queue(queue*);
void enqueue(queue*, int);
int dequeue(queue*);
bool isEmpty(queue*);
int nelem(queue*);
void print (queue*,const char*,int);
void destroy_queue(queue*);
int peek(queue*,int);

#endif
