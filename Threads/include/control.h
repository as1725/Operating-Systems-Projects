#ifndef CONTROL_H
#define CONTROL_H

typedef struct mutex_t
{
    int ticket;
    int turn;
} mutex;

typedef struct semaphore_t
{
    mutex m;
    int value;
} semaphore;

void mutex_init(mutex *m);
void mutex_acquire(mutex *m);
void mutex_release(mutex *m);

void sem_init(semaphore *sem, int value);
void sem_wait(semaphore *sem);
void sem_post(semaphore *sem);

#endif // CONTROL_H