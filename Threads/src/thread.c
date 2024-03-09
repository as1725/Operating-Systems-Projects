#include "thread.h"
#include "jmpbuf-offsets.h"
#include "util.h"
#include "time_control.h"
#include <stdbool.h>

extern struct itimerval timer;
extern sigset_t signal_mask;
thread_t *current_thread = NULL;

thread_t threads[MAX_THREADS];
int thread_id = 0;

typedef struct Node
{
    thread_t *thread;
    struct Node *next;
    int who_blocked_me;
} Node;

typedef struct
{
    Node *front;
    Node *rear;
} Queue;

Queue ready_queue;
Queue blocked_queue;

void initializeQueue(Queue *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

void enqueue(Queue *queue, thread_t *thread, int who_blocked_me)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->thread = thread;
    newNode->next = NULL;

    newNode->who_blocked_me = who_blocked_me;

    if (queue->rear == NULL)
    {
        queue->front = newNode;
        queue->rear = newNode;
    }
    else
    {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

thread_t *dequeue(Queue *queue)
{
    if (queue->front == NULL)
    {
        return NULL;
    }

    Node *temp = queue->front;
    thread_t *thread = temp->thread;

    if (queue->front == queue->rear)
    {
        queue->front = NULL;
        queue->rear = NULL;
    }
    else
    {
        queue->front = queue->front->next;
    }

    free(temp);
    return thread;
}

void block_thread(thread_t *thread, int who_blocked_me)
{
    enqueue(&blocked_queue, thread, who_blocked_me);
}

void unblock_thread(thread_t *thread)
{
    Node *curr = blocked_queue.front;
    while (curr != NULL)
    {
        if (curr->who_blocked_me == thread->thread_id)
        {
            curr->thread->state = READY;
            enqueue(&ready_queue, dequeue(&blocked_queue), -1);
            break;
        }
        curr = curr->next;
    }
}

void init_lib()
{
    initializeQueue(&ready_queue);
    initializeQueue(&blocked_queue);

    for (int i = 0; i < MAX_THREADS; i++)
    {
        threads[i].thread_id = -1;
        threads[i].state = COMPLETED;
    }

    current_thread = NULL;
}

int thread_create(thread_t *thread, void(*start_routine))
{
    int i;
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (threads[i].state == COMPLETED)
            break;
    }

    if (i == MAX_THREADS)
        return -1;

    thread->thread_id = thread_id++;
    thread->state = READY;

    thread->stack = (void *)malloc(sizeof(char *) * STACK_SIZE);
    thread->stack_pointer = thread->stack;

    thread->instruction_pointer = start_routine;

    if (current_thread == NULL)
    {
        current_thread = thread;
        thread->state = RUNNING;
    }
    else
    {
        sigsetjmp(thread->context, 1);
        thread->context->__jmpbuf[JB_RSP] = mangle((unsigned long)thread->stack_pointer + STACK_SIZE);

        thread->context->__jmpbuf[JB_PC] = mangle((unsigned long)thread->instruction_pointer);

        sigemptyset(&thread->context->__saved_mask);

        enqueue(&ready_queue, thread, -1);
    }
    return thread->thread_id;
}

void context_switch(thread_t *prev, thread_t *next)
{
    setitimer(ITIMER_REAL, &timer, NULL);

    sigprocmask(SIG_BLOCK, &signal_mask, NULL);

    if (sigsetjmp(prev->context, 1) == 0)
    {
        if (prev->state != BLOCKED && prev->state != COMPLETED)
        {
            prev->state = READY;
            enqueue(&ready_queue, prev, -1);
        }

        current_thread = next;

        if (next != NULL)
        {
            next->state = RUNNING;

            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            siglongjmp(next->context, 1);
        }
    }
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    return;
}

void thread_yield(void)
{
    scheduler();
}

void scheduler()
{
    thread_t *next_thread = dequeue(&ready_queue);

    if (next_thread == NULL)
    {
        return;
    }

    if (next_thread->state == BLOCKED)
    {
        if (ready_queue.front == NULL)
        {
            context_switch(current_thread, next_thread);
        }
        else
        {
            thread_yield();
        }
    }
    else
    {
        context_switch(current_thread, next_thread);
    }
    return;
}

void thread_exit()
{
    current_thread->state = COMPLETED;

    unblock_thread(current_thread);
    thread_yield();
}

void thread_join(thread_t *thread)
{
    if (thread->state == COMPLETED)
    {
        return;
    }
    current_thread->state = BLOCKED;
    block_thread(current_thread, thread->thread_id);
    thread_yield();
}

void thread_sleep(unsigned int milliseconds)
{
    unsigned int start_time = get_time();
    while (get_time() - start_time < milliseconds)
    {
        ;
    }
}