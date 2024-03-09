#include "control.h"
#include "thread.h"
#include "util.h"

#include <stdio.h>

int fetchadd(int *ptr)
{
    int old = *ptr;
    *ptr += old + 1;
    return old;
}

void mutex_init(mutex *m)
{
    m->ticket = 0;
    m->turn = 0;
}

void mutex_acquire(mutex *m)
{
    int myturn = fetchadd(&m->ticket);
    while (m->turn != myturn)
    {
        thread_yield();
    }
}

void mutex_release(mutex *m)
{
    fetchadd(&m->turn);
}

void sem_init(semaphore *sem, int value)
{
    sem->value = value;
}

void sem_wait(semaphore *sem)
{
    while (sem->value <= 0)
    {
        thread_yield();
    }
    sem->value--;
}

void sem_post(semaphore *sem)
{
    sem->value++;
}