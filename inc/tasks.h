#if !defined(__TASKS_H__)
#define __TASKS_H__

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef void* task_arg;
typedef void* (*task_fn)(void* arg);


typedef struct task_params
{
    bool can_join;
    uint32_t prio;
    uint32_t stack_size;
    task_arg arg;
    task_fn fn;
} task_params_t;

typedef struct task
{
    pthread_t handle;
    
    struct 
    {
        bool started;
        bool running;
    } status;

    struct
    {
        bool joinable;
        uint32_t prio;    
        uint32_t stack_size;    
    } config;

    task_arg arg;               
    task_fn fn;
} task_t;

// Create
task_t * task_create( task_params_t* params );

// Init
void task_init( task_t* task, task_params_t* params );

// Start
void task_start( task_t* task);

// Stop
void task_stop( task_t* task);

void task_deinit( task_t* task);

// Destroy
void task_destroy( task_t* task);



#endif // __TASKS_H__
