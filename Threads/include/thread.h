#include <setjmp.h>

#define STACK_SIZE 4096 // in bytes
#define MAX_THREADS 64  // maximum number of threads


enum THREAD_STATES
{
    RUNNING,
    READY,
    BLOCKED,
    COMPLETED
};

typedef struct thread
{
    int thread_id;
    enum THREAD_STATES state;
    void *instruction_pointer;
    void *stack_pointer;
    void *stack;
    int return_value;

    sigjmp_buf context;

} thread_t;

void init_lib();
int thread_create(thread_t *thread, void(*start_routine));
void thread_exit();
void thread_join(thread_t *thread);
void thread_sleep(unsigned int milliseconds);
void thread_yield(void);
void context_switch(thread_t *prev, thread_t *next);
void scheduler();
