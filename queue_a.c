#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "queue_a.h"

void init_queue(queue *q) {
   q->size = 10;
   q->data = (int*)malloc(q->size*sizeof(int));
   q->head = q->tail = q->nelem = 0;
}

/* void enqueue(queue *q, void *item) { */
void enqueue(queue *q, int item) {
   if ((q->head == 0 && q->tail == q->size-1) || q->head == q->tail + 1) {
      printf("No space left - cannot add - 1\n");
      return;
   }
   q->data[q->tail] = item;
   q->nelem = q->nelem+1;
   if (q->head == q->tail) q->head = q->size-1; 
   if (q->tail == q->size-1) q->tail = 0; else q->tail++;
}

/* void *dequeue(queue *q) { */
int dequeue(queue *q) {
    int object = -1;

    if (q->tail == q->head) 
    {
        return object;
    }
    else if (q->tail > q->head) 
    {
        q->head = q->head+1;
        object = q->data[q->head];
        /* an empty queue, initialize */
        if (q->head+1 == q->tail) 
        {
            q->head = q->tail = 0; 
        }
        q->nelem = q->nelem-1;
        return object;
    }
    else 
    {
        q->head = (q->head+1) % q->size;
        object = q->data[q->head];
        /* an empty queue, initialize */
        if (((q->head+1) % q->size) == q->tail) 
        {
            q->head = q->tail = 0; 
        }
        q->nelem = q->nelem-1;
        return object;
    }
}

bool isEmpty(queue *q) 
{
    return q->tail == q->head;
}

int nelem(queue *q) { return q->nelem; }

void print (queue *q, const char *name, int id) {
    int i;
    char str[4098];

    sprintf(str,"%s (%d): ",name,id);
    if (isEmpty(q)) { 
        sprintf(str,"%s (empty)\n",str);
    } 
    else {
        sprintf(str,"%s [",str);
        for (i=q->head+1 ; i < q->size && i != q->tail ; i++) 
        {
            sprintf(str,"%s %d ",str,q->data[i]);
        }
        if (i == q->size) {
            for (i=0 ; i < q->tail ; i++) {
                sprintf(str,"%s %d ",str,q->data[i]);
            }
        }
        sprintf(str,"%s]\n",str);
    }   
    printf("%s",str); fflush(stdout);
}

int peek (queue *q, int idx) {
   int i;

   if (isEmpty(q)) return -1;
   for (i = q->head+1; i < q->size && i != q->tail && idx > 0; i++, idx--);
   if (idx == 0 && i < q->size) return q->data[i];
   for (i=0 ; i < q->tail && idx > 0 ; i++, idx--);
   if (idx == 0) return q->data[i];
   return -1;
}

void destroy_queue (queue *q) { free(q->data); }
