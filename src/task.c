#include "task.h"
#include <stdlib.h>
#include <string.h>

static void * task_entry_fn( void* args )
{
    task_t* task = (task_t*) args;

    task->status.running = true;
    
    if( task->fn) task->fn( task->arg );
    
    task->status.running = false;

    pthread_exit(NULL);
}

static int task_set_thread_attr( pthread_attr_t* attr, bool joinable,  uint32_t prio, uint32_t stack_size )
{
    int ret;

    // joinable state
    if( (ret = pthread_attr_setdetachstate( attr, joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED )) == 0 )
    {
        
        // stack_size
        if( (ret = pthread_attr_setstacksize( attr, stack_size)) )
        {
            // prio
            struct sched_param sched;
            
            if((ret = pthread_attr_getschedparam( attr, &sched )) == 0 )
            {
                sched.sched_priority = prio;

                if( (ret = pthread_attr_setschedparam( attr, &sched)) )
                {
                    // TODO: Error
                }
            }
            else
            {
                // !Error
            }
        }
        else
        {
            // !Error
        }
    }
    else
    {
        // !Error
    }

    return ret;
}

task_t * task_create( const task_params_t* params )
{
    task_t * task = calloc(1, sizeof(task_t));
    task_init( task, params );
    return task;
}

void task_init( task_t* task, const task_params_t* params )
{
    task->status.running = task->status.started = false;
    
    task->config.joinable = params->can_join;
    task->config.prio = params->prio;
    task->config.stack_size = params->stack_size;

    task->arg = params->arg;
    task->fn = params->fn;
}

// Start
void task_start( task_t* task)
{
    pthread_attr_t attr;
    
    if( pthread_attr_init( &attr) == 0 )
    {
        if(task_set_thread_attr( &attr, task->config.joinable, task->config.prio, task->config.stack_size ) == 0 )
        {
            if(pthread_create( &(task->handle), &attr, task_entry_fn, (void*)task ) == 0 )
            {
                task->status.started = true;
            }
            else
            {
                // ! Error
            }
        }
        else
        {
            // ! Error
        }

        if( pthread_attr_destroy( &attr ) )
        {
            // ! Error
        }
    }
    else
    {
        // ! Error
    }


}

// Stop
void task_stop( task_t* task)
{
    pthread_cancel( task->handle );

    if( task->config.joinable )
    {
        pthread_join( task->handle, NULL );
    }

    task->status.started = false;
}

void task_deinit( task_t* task)
{
    if( task->status.running == false && task->status.started == false )
    {
        task->fn = NULL;
        task->arg = NULL;
    }
    else
    {
        // ! Error
    }
}

// Destroy
void task_destroy( task_t* task)
{
    task_deinit( task );
    free( task );
}