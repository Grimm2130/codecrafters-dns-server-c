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

static const logger_params_t s_logger_params = 
{
    .prior = 10,
    .level = LOG_LEVEL_TRACE,
    .max_log_buf_enttry_cnt = 100,
    .max_file_size = 0x1FFFF,
    .log_fname_ext = "log",
    .log_path = "",
    .log_fname = "logfile",
    .timestamp_formatter = NULL
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
    logger_t logger_inst;
    uint16_t count = 1000;
    log_level* level = (log_level*) arg;

    // Disable cancellation
    pthread_setcancelstate( PTHREAD_CANCEL_DISABLE,  NULL );

    logger_get_instance( &logger_inst, &s_logger_params );
    
    printf("In Default Logger. Inst %u...\n", logger_inst.context_id );
    fflush(stdout);

    while( count-- > 0 )
    {
        switch( *level )
        {
            case LOG_LEVEL_TRACE:
            {
                log_trace( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            }
            break;
            
            case LOG_LEVEL_DEBUG:
            {
                log_debug( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            }
            break;
            
            case LOG_LEVEL_INFO:
            {
                log_info( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            }
            break;
            
            case LOG_LEVEL_ERROR:
            {
                log_error( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            }
            break;
            
            case LOG_LEVEL_FATAL:
            {
                log_fatal( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            }
            break;

            case LOG_LEVEL_DEFAULT:
            default:
            {
                log_trace( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
                log_debug( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
                log_info( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
                log_error( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
                log_fatal( &logger_inst, "Hello from Logger with context id - %u", logger_inst.context_id );
            }
            break;
        }
    }

    printf("In Default Logger. Inst %u...Done\n", logger_inst.context_id );
    
    logger_release_instance( &logger_inst );

    free(level);
}


void run_tests_logger()
{
    logger_init();

    assert( logger_test_single_logger() );
    // assert( logger_test_multi_logger() );
    
    logger_deinit();
}

static bool logger_test_single_logger()
{
    bool ret = false;

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
    
    task_deinit(&s_tasks[0]);
    
    printf("logger_test_single_logger() Exit\n");
    return ret;
}

static bool logger_test_multi_logger()
{
    bool ret = false;
    
    return ret;
}


#endif // __LOGGER_TESTER_H__
