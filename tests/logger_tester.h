#if !defined(__LOGGER_TESTER_H__)
#define __LOGGER_TESTER_H__

#include "logger.h"
#include "task.h"
#include <assert.h>
#include <stdlib.h>

#define NUM_LOGGER_TASKS        10U


static const char *logger_path = "";
static const char *logger_name = "logfile";
static const char *logger_name_ext = "log";

static task_t s_tasks[NUM_LOGGER_TASKS];

void run_tests_logger();
static bool logger_test_single_logger();
static bool logger_test_multi_logger();
void default_logger_thread( void * arg );

static const logger_params_t s_logger_params[NUM_LOGGER_TASKS] = 
{
    {10, LOG_LEVEL_TRACE, 100, 1000000, "log", "", "logfile_1", NULL },
    {10, LOG_LEVEL_DEBUG, 100, 1000000, "log", "", "logfile_2", NULL },
    {10, LOG_LEVEL_INFO, 100, 1000000, "log", "", "logfile_3", NULL },
    {10, LOG_LEVEL_ERROR, 100, 1000000, "log", "", "logfile_4", NULL },
    {10, LOG_LEVEL_FATAL, 100, 1000000, "log", "", "logfile_5", NULL },
    {10, LOG_LEVEL_TRACE, 100, 1000000, "log", "", "logfile_6", NULL },
    {10, LOG_LEVEL_DEBUG, 100, 1000000, "log", "", "logfile_7", NULL },
    {10, LOG_LEVEL_INFO, 100, 1000000, "log", "", "logfile_8", NULL },
    {10, LOG_LEVEL_ERROR, 100, 1000000, "log", "", "logfile_9", NULL },
    {10, LOG_LEVEL_FATAL, 100, 1000000, "log", "", "logfile_10", NULL },
};


static task_params_t s_logger_task_params = {
    .can_join = true,
    .prio = 10,
    .stack_size = (uint32_t)1e6,
    .arg = NULL,
    .fn = default_logger_thread
};

void default_logger_thread( void * arg )
{
    logger_ret_type ret;
    logger_t logger_inst;
    uint16_t count = 1000;
    int* idx = (int*) arg;

    // Disable cancellation
    pthread_setcancelstate( PTHREAD_CANCEL_DISABLE,  NULL );

    ret = logger_get_instance( &logger_inst, &s_logger_params[*idx] );


    if( ret == LOGGER_SUCCESS )
    {
        printf("In Default Logger. Inst %u...\n", logger_inst.context_id ); fflush(stdout);
        
        while( count > 0 )
        {
            printf("Count %u\n", count ); fflush(stdout);
    
            log_trace( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            log_debug( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            log_info( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            log_error( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            log_fatal( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
    
            count--;
        }
    
        printf("In Default Logger. Inst %u...Done\n", logger_inst.context_id );
        
        logger_release_instance( &logger_inst );

    }

    free(idx);
}


void run_tests_logger()
{
    logger_init();

    assert( logger_test_single_logger() );
    assert( logger_test_multi_logger() );
    
    logger_deinit();
}

static bool logger_test_single_logger()
{
    bool ret = true;

    printf("logger_test_single_logger() Enter\n");
    fflush(stdout);
    
    log_level *level = calloc(1, sizeof(log_level));
    *level = LOG_LEVEL_DEFAULT;
    s_logger_task_params.arg = level;
    
    printf("logger_test_single_logger() Initializing task\n");
    fflush(stdout);
    task_init(&s_tasks[0], &s_logger_task_params);
    
    printf("logger_test_single_logger() Starting task\n");
    fflush(stdout);
    task_start( &s_tasks[0] );

    printf("logger_test_single_logger() Stopping task\n");
    task_stop( &s_tasks[0] );
    
    printf("logger_test_single_logger() Deinitializing task\n");
    task_deinit(&s_tasks[0]);
    
    printf("logger_test_single_logger() Exit\n");
    
    return ret;
}

static bool logger_test_multi_logger()
{
    bool ret = true;

    printf("logger_test_multi_logger() Enter\n"); fflush(stdout);
    
    for( uint8_t i = 0; i <  NUM_LOGGER_TASKS; i++ )
    {
        int *idx = calloc(1, sizeof(int));
        *idx = i;
        s_logger_task_params.arg = idx;
        
        printf("logger_test_multi_logger() Initializing task\n"); fflush(stdout);
        task_init(&s_tasks[i], &s_logger_task_params);
        
        printf("logger_test_multi_logger() Starting task\n"); fflush(stdout);
        task_start( &s_tasks[i] );
    }
    
    for( uint8_t i = 0; i <  NUM_LOGGER_TASKS; i++ )
    {
        printf("logger_test_multi_logger() Stopping task\n"); fflush(stdout);
        task_stop( &s_tasks[i] );
        
        printf("logger_test_multi_logger() Deinitializing task\n"); fflush(stdout);
        task_deinit(&s_tasks[i]);
    }
    
    printf("logger_test_single_logger() Exit\n");
    
    return ret;
}


#endif // __LOGGER_TESTER_H__
