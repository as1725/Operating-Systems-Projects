#include "thread.h"
#include "control.h"
#include "time_control.h"
#include "util.h"

typedef struct
{
    int id;
    int arrival_time;
    int treatment_time;
} Patient;

int num_patients;
Patient *patients;
mutex doctor_mutex;
extern thread_t *current_thread;

void patient_arrival(int id)
{
    LOG_OUT("Patient %d got in line\n", id);

    int arrival_time = patients[id - 1].arrival_time;
    if (arrival_time > 0)
    {
        thread_sleep(arrival_time);
    }
}

void patient_treatment(int id)
{
    int treatment_time = patients[id - 1].treatment_time;

    mutex_acquire(&doctor_mutex);

    LOG_OUT("Patient %d starting seeing the doctor\n", id);
    thread_sleep(treatment_time);

    LOG_OUT("Patient %d finished seeing the doctor\n", id);
    mutex_release(&doctor_mutex);
}

void *patient()
{
    int id = patients[current_thread->thread_id - 1].id;

    patient_arrival(id);
    patient_treatment(id);

    thread_exit();
}

int main()
{
    init_lib();
    thread_t *main_t = malloc(sizeof(thread_t));

    scanf("%d", &num_patients);

    mutex_init(&doctor_mutex);
    patients = malloc(sizeof(Patient) * num_patients);

    for (int i = 0; i < num_patients; i++)
    {
        scanf("%d %d", &patients[i].arrival_time, &patients[i].treatment_time);
        patients[i].id = i + 1;
    }

    thread_create(main_t, NULL);

    thread_t *patient_threads[num_patients];
    for (int i = 0; i < num_patients; i++)
    {
        patient_threads[i] = malloc(sizeof(thread_t));
        thread_create(patient_threads[i], &patient);
    }

    timer_start();
    thread_yield();

    for (int i = 0; i < num_patients; i++)
    {
        thread_join(patient_threads[i]);
    }

    LOG_OUT("All patients have left\n");
    timer_stop();
    return 0;
}
