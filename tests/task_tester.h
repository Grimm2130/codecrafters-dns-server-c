#if !defined(__TAST_TESTER_H__)
#define __TAST_TESTER_H__

#include "task.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define NUM_TASKS 10

task_params_t param[NUM_TASKS];
task_t tasks[NUM_TASKS];

static void task_test_routines(void *arg)
{
    FILE *fp;
    char fullPath[100];
    uint64_t iters = *(uint64_t *)arg;

    snprintf(fullPath, 100, "test.log.task.log.%llu.log", iters);

    if ((fp = fopen(fullPath, "w")))
    {
        for (uint64_t i = 0; i < iters; i++)
        {
            fprintf(fp, "Entry %llu\n", i);
        }

        fclose(fp);
    }
}

void run_tests_task();

static bool init_tasks();
static bool start_tasks();
static bool stop_tasks();
static bool deinit_tasks();

void run_tests_task()
{
    assert(init_tasks());
    assert(start_tasks());
    assert(stop_tasks());
    assert(deinit_tasks());
}

static bool init_tasks()
{
    uint64_t *arg;
    for (int i = 0; i < NUM_TASKS; i++)
    {
        param[i].can_join = true;
        param[i].prio = 10;
        param[i].stack_size = 1024;
        param[i].fn = task_test_routines;
        arg = (uint64_t *)malloc(sizeof(u_int64_t));
        *arg = (i + 1) * 1e3;
        param[i].arg = (void *)arg;

        task_init(&tasks[i], &param[i]);
    }

    return true;
}

static bool start_tasks()
{
    for (int i = 0; i < NUM_TASKS; i++)
    {
        task_start(&tasks[i]);
    }

    return true;
}

static bool stop_tasks()
{
    for (int i = 0; i < NUM_TASKS; i++)
    {
        task_stop(&tasks[i]);
    }

    return true;
}

static bool deinit_tasks()
{
    for (int i = 0; i < NUM_TASKS; i++)
    {
        free(param[i].arg);
        task_deinit(&tasks[i]);
    }

    return true;
}

#endif // __TAST_TESTER_H__
