#if !defined(__SYNC_CIR_BUFFER_TESTER_H__)
#define __SYNC_CIR_BUFFER_TESTER_H__

#include "tasks.h"
#include "sync_cir_buffer.h"
#include "students.h"

#include <stdio.h>
#include <unistd.h>

#define ITERS 300

sync_cir_buffer_t buffer;
task_t producer, consumer;

void run_test_sync_cir_buffer();
static void init();
static void deinit();
static void read_student(student_t *st);
static void write_student(const student_t *st);

void run_test_sync_cir_buffer()
{
    init();
    deinit();
}

void *producer_routine(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    for (int i = 0; i < ITERS; i++)
    {
        // printf("Attempting Write...\n");
        write_student(&students[i]);
        printf("Wrote: ");
        printf(STUDENT_STR((&students[i])));
        // sleep(1);
    }
    fflush(stdout);

    return NULL;
}

void *consumer_routine(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    for (int i = 0; i < ITERS; i++)
    {
        // printf("Attempting read...\n");
        read_student(&students[i]);
        printf("Read: ");
        printf(STUDENT_STR((&students[i])));
        // sleep(1);
    }
    fflush(stdout);

    return NULL;
}

static void init()
{
    sync_cir_buffer_init(&buffer, true);

    task_params_t p_param =
        {
            .can_join = true,
            .prio = 10,
            .stack_size = 1000,
            .arg = NULL,
            .fn = producer_routine};
    task_params_t c_param =
        {
            .can_join = true,
            .prio = 10,
            .stack_size = 1000,
            .arg = NULL,
            .fn = consumer_routine};

    task_init(&producer, &p_param);
    task_init(&consumer, &c_param);

    task_start(&producer);
    task_start(&consumer);
}

static void deinit()
{
    // task_stop( &producer );
    task_stop(&consumer);

    task_deinit(&producer);
    task_deinit(&consumer);

    sync_cir_buffer_deinit(&buffer);
}

static void read_student(student_t *st)
{
    sync_cir_buffer_read(&buffer, (char *)st, STUDENT_SIZE, false);
}

static void write_student(const student_t *st)
{
    sync_cir_buffer_write(&buffer, (const char *)st, STUDENT_SIZE, false);
}

#endif // __SYNC_CIR_BUFFER_TESTER_H__
