#include "thread.h"
#include "control.h"
#include "util.h"
#include "time_control.h"

int n;
int m;
semaphore machines;
extern thread_t *current_thread;

typedef struct Customer
{
    int id;
    int time;
} Customer;
Customer *customers;

void customer_arrives(int id)
{
    LOG_OUT("Customer %d got in line\n", id + 1);
    sem_wait(&machines);
}

void customer_washes(int id)
{
    int time = customers[id].time;

    LOG_OUT("Customer %d starting washing their clothes\n", id + 1);
    thread_sleep(time);

    LOG_OUT("Customer %d finished washing their clothes\n", id + 1);
    sem_post(&machines);
}

void *customer()
{
    int id = customers[current_thread->thread_id - 1].id;

    customer_arrives(id);
    customer_washes(id);

    thread_exit();
}

int main()
{
    init_lib();
    thread_t *main_t = malloc(sizeof(thread_t));

    scanf("%d %d", &n, &m);

    sem_init(&machines, n);

    customers = malloc(sizeof(Customer) * m);

    for (int i = 0; i < m; i++)
    {
        scanf("%d", &(customers[i].time));
    }

    thread_create(main_t, NULL);
    thread_t *customer_threads[m];
    for (int i = 0; i < m; i++)
    {
        customers[i].id = i;
        customer_threads[i] = malloc(sizeof(thread_t));
        thread_create(customer_threads[i], &customer);
    }

    timer_start();
    thread_yield();

    for (int i = 0; i < m; i++)
    {
        thread_join(customer_threads[i]);
    }

    LOG_OUT("All customers have left\n");
    timer_stop();

    return 0;
}
