#include <stdlib.h>
#include <pthread.h>
// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;

    pthread_mutex_t *mutexQueue;
    pthread_cond_t full;
    pthread_cond_t empty;
} _queue;


#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));

    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data  = malloc(size*sizeof(void *));

    q->mutexQueue = malloc(sizeof (pthread_mutex_t));

    pthread_mutex_init(q->mutexQueue , NULL);
    pthread_cond_init(&q->empty , NULL);
    pthread_cond_init(&q->full , NULL);

    return q;
}

int q_elements(queue q) {
    return q->used;
}

int q_insert(queue q, void *elem) {///productor(codigo productor con condiciones (transparencias))

    pthread_mutex_lock(q->mutexQueue);
    while(q->size == q->used)
        pthread_cond_wait(&q->full , q->mutexQueue);

    q->data[(q->first+q->used) % q->size] = elem;
    q->used++;

    if(q->used > 0)///
        pthread_cond_broadcast(&q->empty);
    pthread_mutex_unlock(q->mutexQueue);

    return 1;
}///productor

void *q_remove(queue q) {///consumidor
    void *res;

    pthread_mutex_lock(q->mutexQueue);
    while(q->used==0)       ///detectar cuando está vacia y ya no existen threads q vayan a meter cosas nuevas para sacalo de aqui /usar limte de tempo
        pthread_cond_wait(&q->empty , q->mutexQueue);

    res = q->data[q->first];
    q->first = (q->first+1) % q->size;
    q->used--;

    if(q->used < q->size)
        pthread_cond_broadcast(&q->full);
    pthread_mutex_unlock(q->mutexQueue);

    return res;
}///consumidor

void q_destroy(queue q) {

    pthread_cond_destroy(&q->empty);
    pthread_cond_destroy(&q->full);
    pthread_mutex_destroy(q->mutexQueue);

    free(q->data);
    free(q);
}