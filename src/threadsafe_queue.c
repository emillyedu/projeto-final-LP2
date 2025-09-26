#include "threadsafe_queue.h"
#include <stdlib.h>
#include <errno.h>
#include <time.h>

static void timespec_add_ms(struct timespec *ts, int ms) {
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_nsec += (long)ms * 1000000L;
    while (ts->tv_nsec >= 1000000000L) { ts->tv_nsec -= 1000000000L; ts->tv_sec += 1; }
}

tsq_t* tsq_create(size_t capacity) {
    tsq_t *q = (tsq_t*)calloc(1, sizeof(tsq_t));
    if (!q) return NULL;
    q->buf = (char**)calloc(capacity, sizeof(char*));
    if (!q->buf) { free(q); return NULL; }
    q->cap = capacity;
    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->can_push, NULL);
    pthread_cond_init(&q->can_pop, NULL);
    return q;
}

void tsq_close(tsq_t* q) {
    if (!q) return;
    pthread_mutex_lock(&q->mtx);
    q->closed = 1;
    pthread_cond_broadcast(&q->can_pop);
    pthread_cond_broadcast(&q->can_push);
    pthread_mutex_unlock(&q->mtx);
}

void tsq_destroy(tsq_t* q) {
    if (!q) return;
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->can_push);
    pthread_cond_destroy(&q->can_pop);
    free(q->buf);
    free(q);
}

int tsq_push(tsq_t* q, char* msg, int block_ms) {
    if (!q) return -1;
    int ret = 0;
    pthread_mutex_lock(&q->mtx);
    if (q->closed) { ret = -1; goto out; }
    while (!q->closed && q->count == q->cap) {
        if (block_ms <= 0) { ret = -1; goto out; }
        struct timespec ts; timespec_add_ms(&ts, block_ms);
        int rc = pthread_cond_timedwait(&q->can_push, &q->mtx, &ts);
        if (rc == ETIMEDOUT) { ret = -1; goto out; }
    }
    if (q->closed) { ret = -1; goto out; }
    q->buf[q->tail] = msg;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    pthread_cond_signal(&q->can_pop);
out:
    pthread_mutex_unlock(&q->mtx);
    return ret;
}

char* tsq_pop(tsq_t* q) {
    if (!q) return NULL;
    pthread_mutex_lock(&q->mtx);
    while (q->count == 0 && !q->closed) {
        pthread_cond_wait(&q->can_pop, &q->mtx);
    }
    if (q->count == 0 && q->closed) {
        pthread_mutex_unlock(&q->mtx);
        return NULL;
    }
    char* msg = q->buf[q->head];
    q->buf[q->head] = NULL;
    q->head = (q->head + 1) % q->cap;
    q->count--;
    pthread_cond_signal(&q->can_push);
    pthread_mutex_unlock(&q->mtx);
    return msg;
}