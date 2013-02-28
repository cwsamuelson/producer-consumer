/*
    Producer and Consumer model with 1 producer and any number of consumers.
    Written by John Eversole and Chris Samuelson
    We did it!
*/

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "queue_a.h"

#define BUFFER_SIZE 15
int idcnt = 1;
queue workq;

/* One of these per stream.
   Holds:  the mutex lock and notifier condition variables
           a buffer of tokens taken from a producer
           a structure with information regarding the processing of tokens 
           an identity
*/
typedef struct stream_struct {
    struct stream_struct *next;
    pthread_mutex_t lock;
    pthread_cond_t notifier;
    queue buffer;               /* a buffer of values of any type      */
    void *args;                 /* arguments for the producing stream */
    int id;                     /* identity of this stream            */
    int current;
    int num_consumers;
    int last_consumed;
    int consumers;
} Stream;

/* prod: linked list of streams that this producer consumes tokens from
   self: the producer's output stream */
typedef struct {
   Stream *self, *prod;
} Args;

void remove_unneeded_items(Stream *producer) {
    int num_of_dequeues = 0;
    Stream *node = producer;
    queue *q = &node->buffer;
    int lowest_consumed = 4000;
    
    // no consumers connected
    if (node->next == NULL) {
        return;
    }

    // gather lowest value consumed
    while (node->next != NULL) {
        node = node->next;    
        if (node->last_consumed <= lowest_consumed) {
            lowest_consumed = node->last_consumed;
        }
    }
    
    // as long as the first value has been consumed by all
    while (peek(q, 0) <= lowest_consumed && !isEmpty(q)) {
        dequeue(q);
    }
}

/* return 'value' which should have a value from a call to put */
int get(void *streams, int last_consumed) {
    int offset = 1;
    int ret;
    bool need_token;

    Args *args = (Args *)streams;
    Stream *prod = (Stream *)args->prod;
    Stream *con = (Stream *)args->self;
    queue *q = &prod->buffer;
    pthread_mutex_t *lock = &prod->lock;
    pthread_cond_t *notifier = &prod->notifier;

    pthread_mutex_lock(lock);

    need_token = (isEmpty(q) || peek(q, nelem(q) - 1) <= last_consumed);

    // clean out unneeded tokens
    remove_unneeded_items(prod);

    // if empty or the newest token is <= last thing we consumed
    if (need_token) {
        // add a token job to the work queue
        enqueue(&workq, last_consumed);

        while(need_token) {
            // wait for producer to wake us up
            pthread_cond_wait(notifier, lock);
            // check if we still need it
            need_token = (isEmpty(q) || peek(q, nelem(q) - 1) <= last_consumed);
        }

        ret = peek(q, nelem(q) - offset);
        while (ret - con->last_consumed != 1) {
            offset = offset + 1;
            ret = peek(q, nelem(q) - offset);
        }
        
        /*
        // grab the most recent token
        ret = peek(q, nelem(q) - 1);
        if (ret - con->last_consumed > 1) {
            ret = peek(q, nelem(q) - 2);
            printf("id: %d last_consumed: %d ret: %d\n", con->id, con->last_consumed, ret);
            print(q, "assert", 0);
            assert(0);
        }
        */
    }
    // token in buffer
    else if (nelem(q) - last_consumed <= BUFFER_SIZE) {
        //int x = peek(q, nelem(q) - 1) - last_consumed;
        //ret = peek(q, nelem(q) - 1);
        ret = peek(q, nelem(q) - offset);
        while (ret - con->last_consumed != 1) {
            offset = offset + 1;
            ret = peek(q, nelem(q) - offset);
        }
    }
    
    con->last_consumed = ret;
    pthread_mutex_unlock(lock);
    
    return ret;
}

/* 'value' is the value to move to the consumer */
bool put(Stream *prod, int value) {
    queue *q = &prod->buffer;
    pthread_mutex_t *lock = &prod->lock;
    int token_last_consumed;
    bool value_was_put = false;

    pthread_mutex_lock(lock);

    // clean out unneeded tokens
    remove_unneeded_items(prod);

    // remove that job from the workqueue
    token_last_consumed = dequeue(&workq);

    // new token should be added onto the producer buffer
    if (isEmpty(q) || (token_last_consumed == peek(q, nelem(q) - 1))) {
        enqueue(q, value);
        value_was_put = true;
    }
    
    pthread_mutex_unlock(lock);

    return value_was_put;
}

void *producer (void *streams) {
    Stream *prod = ((Args*)streams)->self;
    int id = ((Args*)streams)->self->id;
    int token_count = 1;
    int token_from_job;
    bool was_put;

    while (true) {
        if (!isEmpty(&workq)) {
            was_put = put(prod, token_count);
            if (was_put == true) {
                printf("Producer(%d): sent %d, buf_sz=%d\n",
                    id, token_count, nelem(&prod->buffer));
                token_count++;
            }
            pthread_cond_broadcast(&prod->notifier);
        }
    }

    pthread_exit(NULL);
}

/* Final consumer in the network */
void *consumer (void *streams) {
    Stream *self = ((Args*)streams)->self;
    Stream *prod = ((Args*)streams)->prod;
    int i;
    int value;

    for (i=0 ; i < 5; i++) {
        value = get(streams, self->last_consumed); 
        self->last_consumed = value;
        printf("\t\t\t\t\t\t\tConsumer (%d): got %d\n", self->id, value);
    }

    pthread_exit(NULL);
}

/* initialize streams - see also queue_a.h and queue_a.c */
void init_stream (Args *args, Stream *self, void *data) {
    if (self != NULL) {
        self->next = NULL;
        self->args = data;
        self->id = idcnt++;
        self->num_consumers = 0;
        self->last_consumed = 0;
        init_queue(&self->buffer);
        pthread_mutex_init(&self->lock, NULL);
        pthread_cond_init (&self->notifier, NULL);
    }
    args->self = self;
    args->prod = NULL;
}

/* free allocated space in the queue - see queue_a.h and queue_a.c */
void kill_stream(Stream *stream) 
{   
    destroy_queue(&stream->buffer);
}

/* puts an initialized stream object onto the end of a stream's input list */
void connect (Args *arg, Stream *s) {  
    // no other consumers
    if (s->next == NULL)
    {
        // make consumer first element of linked list
        s->next = arg->self;    
    }
    // consumers already exist
    else
    {
        // start at first consumer in linked list
        Stream *node = s->next;

        // find the end of the linked list
        while (node->next != NULL)
        {
            // advance
            node = node->next;
        }
        // tack on the new consumer
        node->next = arg->self;
    }
    arg->prod = s;
    s->num_consumers++;
}

int main () {
    pthread_t s1, c1, c2, c3, c4;
    Stream suc1, con1, con2, con3, con4;
    Args suc1_args, cons1_args, cons2_args, cons3_args, cons4_args;
    pthread_attr_t attr;

    init_queue(&workq);

    init_stream(&suc1_args, &suc1, NULL);

    init_stream(&cons1_args, &con1, NULL);
    connect(&cons1_args, &suc1);

    init_stream(&cons2_args, &con2, NULL);
    connect(&cons2_args, &suc1);

    init_stream(&cons3_args, &con3, NULL);
    connect(&cons3_args, &suc1);

    init_stream(&cons4_args, &con4, NULL);
    connect(&cons4_args, &suc1);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&s1, &attr, producer, (void*)&suc1_args);
    pthread_create(&c1, &attr, consumer, (void*)&cons1_args);
    pthread_create(&c2, &attr, consumer, (void*)&cons2_args);
    pthread_create(&c3, &attr, consumer, (void*)&cons3_args);
    pthread_create(&c4, &attr, consumer, (void*)&cons4_args);

    pthread_join(c4, NULL);
    pthread_join(c3, NULL);
    pthread_join(c2, NULL);
    pthread_join(c1, NULL);

    exit(0);
}
   
