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

#define CHUNKS  (1500 * 64)
#define MAX_THR 1

void* gp[CHUNKS];

void ut_task(void* p)
{

    void* f   = NULL;
    int   hit = 0;

    for (int i = 0; i < CHUNKS; i++)
    {
        f = alloc_fragment(8);
        if (f)
        {
            hit++;
            gp[i] = f;
        }
    }
    DBG("Get hit:%d\n", hit);
    for (int i = 0; i < hit; i++)
    {
        free_fragment(gp[i]);
        // DBG("Excp:%d,%p\n", i, gp[i]);
    }
    hit = 0;
    DBG("=============\n");
    for (int i = 0; i < CHUNKS; i++)
    {
        f = alloc_fragment(8);
        if (f)
        {
            hit++;
            gp[i] = f;
        }
    }
    DBG("Get hit:%d\n", hit);
    for (int i = 0; i < hit; i++)
    {
        free_fragment(gp[i]);
        // DBG("Excp:%d,%p\n", i, gp[i]);
    }
    DBG("Done");
    return;
}

int main(int argc, char** argv)
{
    pthread_t ntid[MAX_THR];
    for (int i = 0; i < MAX_THR; i++)
    {
        pthread_create(&ntid[i], NULL, (void* (*)(void*))ut_task, (void*)i);
    }

    for (int i = 0; i < MAX_THR; i++)
    {
        pthread_join(ntid[i], NULL);
    }
}
