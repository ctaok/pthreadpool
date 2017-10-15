#include "pthreadpool.h"

void *myprocess(void *arg)
{
    sleep(1);
    printf("threadid is 0x%x, working on task %d\n", pthread_self (),*(int *) arg);
    fflush(stdout);
    sleep(1);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("usage: ./bin thread_num task_num!\n");
        return -1;
    }
    int thread_num = atoi(argv[1]);
    int task_num = atoi(argv[2]);
    int barrier_num = task_num >= thread_num ? thread_num + 1 : task_num + 1;
    pool_init(thread_num, task_num);

    int *workingnum = (int*)malloc(sizeof (int) * task_num);
    int i;
    pthread_barrier_init(&_pool->barrier, NULL, barrier_num);
    for (i = 0; i < task_num; i++) {
        workingnum[i] = i;
        pool_add_worker (myprocess, &workingnum[i]);
    }
    pthread_barrier_wait(&_pool->barrier);
    printf("work over\n");
    pool_destroy();
    free(workingnum);
    return 0;
}
