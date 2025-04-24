#define _GNU_SOURCE
#include "assert.h"
#include "ffa.h"
#include "ffa_core.h"
#include <pthread.h>
#include <sched.h>
#include <time.h>

void pin_to_cpu(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1)
    {
        perror("sched_setaffinity");
    }
}

static inline uint64_t get_time_now()
{
    struct timespec mtime;
    clock_gettime(CLOCK_MONOTONIC, &mtime);
    return mtime.tv_nsec;
}

static inline uint64_t get_cpu_time()
{
    struct timespec mtime;
    clock_gettime(CLOCK_MONOTONIC, &mtime);
    return mtime.tv_nsec;
    // struct cpu_time {
    //     uint64_t tsc;
    //     uint32_t cpu_id;
    // };
    // struct cpu_time result;
    // unsigned int    aux;

    // asm volatile("rdtscp" : "=a"(result.tsc), "=d"(aux),
    // "=c"(result.cpu_id)); result.tsc = (((uint64_t)aux) << 32) | result.tsc;
    // return result.tsc;
}

#define CHUNKS  1000000
#define MAX_THR 1

void* gp[CHUNKS];

void mesure_task(void* p)
{

    //    pin_to_cpu((uint64_t)p);

    const size_t sz = 1024;
    ;
    uint64_t cycle_sum_alloc = 0, cycle_sum_free = 0, cyclediff = 0;
    int      hit = 0;
    void*    f   = NULL;
    for (int i = 0; i < CHUNKS; i++)
    {
        f = alloc_fragment(sz);
        // f = new char(sz);
        f     = malloc(sz);
        gp[i] = f;
    }

    cyclediff = get_cpu_time();
    for (int i = 0; i < CHUNKS; i++)
    {
        free_fragment((gp[i]));
        // delete gp[i];
        // free(gp[i]);
    }
    cyclediff = get_cpu_time() - cyclediff;

    printf("Avg time per %.2f size chunks for %d chunks.\n",
           (double)cyclediff / CHUNKS, CHUNKS);
    return;
}

int main(int argc, char** argv)
{
    pthread_t ntid[MAX_THR];
    for (int i = 0; i < MAX_THR; i++)
    {
        pthread_create(&ntid[i], NULL, (void* (*)(void*))mesure_task, (void*)i);
    }

    for (int i = 0; i < MAX_THR; i++)
    {
        pthread_join(ntid[i], NULL);
    }
}
