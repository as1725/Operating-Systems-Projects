#include "time_control.h"
#include "util.h"
#include "thread.h"

// the timer for the alarm clock
struct itimerval timer;

/*
    mask used to ignore some signals
    we need to ignore the alarm clock signal when we are doing critical stuff
    an empty masks means all signals are enabled
*/
sigset_t signal_mask;
void scheduler();

void interrupt_generated(int signum);

int get_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);

    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

void timer_start()
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = TIME_SLICE;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = TIME_SLICE;

    // when a time slice is over, call the scheduler function
    // by raising the SIGALRM signal
    signal(SIGALRM, interrupt_generated);

    // this mask will be used to mask the alarm signal when we are
    // performing critical tasks that cannot be left midway
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGALRM); // add only the alarm signal

    // start the timer
    setitimer(ITIMER_REAL, &timer, NULL);
}

void timer_stop()
{
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);
}

void interrupt_generated(int signum)
{
    // context_switch(current_thread, dequeue(&ready_queue));
    if (signum == SIGALRM)
        scheduler();
    // write_formatted("Interrupt generated: %d\n", signum);
}