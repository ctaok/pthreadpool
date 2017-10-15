#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>

typedef struct worker {
    void *(*process) (void *arg);
    void *arg;
    struct worker *next;
} CThread_worker;

typedef struct pool {
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;
    pthread_barrier_t barrier;
    pthread_t *threadid;
    CThread_worker *queue_head;
    int shutdown;
    int max_thread_num;
    int task_num;
    int cur_queue_size;
} CThread_pool;

static CThread_pool *_pool = NULL;

int pool_add_worker(void *(*process) (void *arg), void *arg)
{
    CThread_worker *newworker = (CThread_worker *)malloc(sizeof (CThread_worker));
    newworker->process = process;
    newworker->arg = arg;
    newworker->next = NULL;
    pthread_mutex_lock(&(_pool->queue_lock));
    CThread_worker *member = _pool->queue_head;
    if (member != NULL) {
        while (member->next != NULL)
            member = member->next;
        member->next = newworker;
    } else {
        _pool->queue_head = newworker;
    }
    assert (_pool->queue_head != NULL);
    _pool->cur_queue_size++;
    pthread_mutex_unlock(&(_pool->queue_lock));
    pthread_cond_signal(&(_pool->queue_ready));
    return 0;
}

void *thread_routine(void *arg)
{
    printf("starting thread 0x%x\n", pthread_self ());
    fflush(stdout);
    while (1) {
        int set_barrier = 0;
        pthread_mutex_lock(&(_pool->queue_lock));
        while (_pool->cur_queue_size == 0 && !_pool->shutdown) {
            printf("thread 0x%x is waiting\n", pthread_self ());
            fflush(stdout);
            pthread_cond_wait(&(_pool->queue_ready), &(_pool->queue_lock));
        }
        if (_pool->shutdown) {
            pthread_mutex_unlock(&(_pool->queue_lock));
            printf("thread 0x%x will exit\n", pthread_self ());
            fflush(stdout);
            pthread_exit (NULL);
        }
        printf("thread 0x%x is starting to work\n", pthread_self ());
        fflush(stdout);
        assert(_pool->cur_queue_size != 0);
        assert(_pool->queue_head != NULL);
        if (_pool->task_num <= _pool->max_thread_num) {
            printf("%dvs%d\n", _pool->task_num, _pool->max_thread_num);
            fflush(stdout);
            set_barrier = 1;
        }
        _pool->cur_queue_size--;
        _pool->task_num--;
        CThread_worker *worker = _pool->queue_head;
        _pool->queue_head = worker->next;
        pthread_mutex_unlock (&(_pool->queue_lock));
        (*(worker->process)) (worker->arg);
        free (worker);
        worker = NULL;
        if (set_barrier) {
            set_barrier = 0;
            printf("thread 0x%x wait for barrier\n", pthread_self ());
            pthread_barrier_wait(&_pool->barrier);
        }
    }
    pthread_exit(NULL);
}

void pool_init(int max_thread_num, int task_num)
{
    int i = 0;
    _pool = (CThread_pool *)malloc(sizeof (CThread_pool));
    pthread_mutex_init(&(_pool->queue_lock), NULL);
    pthread_cond_init(&(_pool->queue_ready), NULL);
    _pool->queue_head = NULL;
    _pool->max_thread_num = max_thread_num;
    _pool->task_num = task_num;
    _pool->cur_queue_size = 0;
    _pool->shutdown = 0;
    _pool->threadid = (pthread_t *)malloc(max_thread_num * sizeof (pthread_t));
    for (i = 0; i < max_thread_num; i++) {
        pthread_create(&(_pool->threadid[i]), NULL, thread_routine, NULL);
    }
}

int pool_destroy()
{
    int i;
    if (_pool->shutdown)
        return -1;
    _pool->shutdown = 1;
    sleep(1);
    pthread_cond_broadcast(&(_pool->queue_ready));
    for (i = 0; i < _pool->max_thread_num; i++)
        pthread_join (_pool->threadid[i], NULL);
    free(_pool->threadid);
    CThread_worker *head = NULL;
    while (_pool->queue_head != NULL) {
        head = _pool->queue_head;
        _pool->queue_head = _pool->queue_head->next;
        free(head);
    }
    pthread_mutex_destroy(&(_pool->queue_lock));
    pthread_cond_destroy(&(_pool->queue_ready));
    free(_pool);
    _pool = NULL;
    return 0;
}
