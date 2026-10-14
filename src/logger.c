#include "logger.h"
#include "sync_cir_buffer.h"
#include "task.h"
#include "file_io.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#define     LOG_PATH_LEN            255U
#define     LOG_FNAME_LEN           255U
#define     LOG_FNAME_EXT_LEN       5U
#define     LOG_MSG_LEN             1024U
#define     LOG_ENTRY_STR_LEN           (LOG_MSG_LEN + 100)
#define     MAX_LOGGER_CONTEXTS     100
#define     LOG_WRITE_BUFFER_SIZE   (0xFFFF)

#define     LOG_WRITTER_PRIO        20U

#define     INVALID_LOGGER_CONTEXT_ID   MAX_LOGGER_CONTEXTS + 1

#define  _LOCK(lock_ptr) (pthread_mutex_lock(&(lock_ptr)));
#define  _UNLOCK(lock_ptr) (pthread_mutex_unlock(&(lock_ptr)));

#define  RD_LOCK(lock_ptr) (pthread_rwlock_rdlock(&(lock_ptr)));
#define  WR_LOCK(lock_ptr) (pthread_rwlock_wrlock(&(lock_ptr)));
#define  RW_UNLOCK(lock_ptr) (pthread_rwlock_unlock(&(lock_ptr)));

#define MGR_SIGNAL_READ(cv_ptr) (pthread_cond_signal(&(cv_ptr)))
#define MGR_BROADCAST_READ(cv_ptr) (pthread_cond_broadcast(&(cv_ptr)))
#define MGR_WAIT_READ(cv_ptr, lock_ptr) (pthread_cond_wait(&(cv_ptr) &(lock_ptr)))


// typedef atomic_uint_fast8_t     atomic_u8;
// typedef atomic_uint_fast16_t    atomic_u16;
// typedef atomic_uint_fast32_t    atomic_u32;
// typedef atomic_uint_fast64_t    atomic_u64;

typedef struct log_timestamp
{   
    uint64_t raw;
} log_timestamp_t;

/// @brief Log entry
typedef struct log_entry
{
    log_timestamp_t timestamp;
    log_level level;
    char msg[LOG_MSG_LEN];
} log_entry_t;


/// @brief Logger context
/// This is a hidden object that stores information about the logger object
typedef struct logger_context
{
    atomic_bool _init;
    uint8_t prior;
    log_level level;
    uint16_t entry_count;
    uint16_t entry_buffer_cap;
    uint32_t file_size;
    uint32_t max_file_size;                 // max file size in bytes
    pthread_mutex_t lock;
    char log_fname_ext[LOG_FNAME_EXT_LEN];
    char log_path[LOG_PATH_LEN];
    char log_fname[LOG_FNAME_LEN];
    file_t file;
    sync_cir_buffer_t *entry_queue;
    logger_time_stamp_formatter timestamp_formatter;
} logger_context_t;

typedef struct logger_context_mgr
{
    // struct
    // {
    //     uint16_t len;
    //     char log_write_buffer[LOG_WRITE_BUFFER_SIZE];
    // } write_buffer;

    task_t *_log_writer;
    logger_context_t * contexts[ MAX_LOGGER_CONTEXTS ];
}logger_context_mgr_t;

/*
*	================================== Function Declarations ==================================
*/


static void logger_context_mgr_init( );
static void logger_context_mgr_deinit( );


static logger_ret_type logger_context_mgr_register_context( logger_t *, const logger_params_t* );
static logger_ret_type logger_context_mgr_unregister_context( logger_t * );
static logger_ret_type logger_context_mgr_append_log( logger_t *, log_entry_t * );
static void logger_context_mgr_set_log_level( logger_t *, log_level );
static const log_level logger_context_mgr_get_log_level( logger_t * );

static logger_ret_type logger_context_create( logger_context_t**, const logger_params_t* );
static logger_ret_type logger_context_init( logger_context_t*, const logger_params_t* );
static bool logger_context_ready_to_write( logger_context_t* );
static void logger_context_set_log_level( logger_context_t*, const log_level );
static log_level logger_context_get_log_level( logger_context_t* );
static bool logger_context_open_file(logger_context_t *);
static bool logger_context_close_file(logger_context_t *);
static void logger_context_append_log( logger_context_t*, log_entry_t * );
static void logger_context_write_log( logger_context_t* );
static void logger_context_flush_logs( logger_context_t* );
static logger_ret_type logger_context_deinit( logger_context_t* );
static logger_ret_type logger_context_destroy( logger_context_t** );

static const char* log_level_to_str( const log_level level);

static uint32_t log_entry_to_str( const log_entry_t * entry, char *out_buf, uint32_t size );

static uint8_t default_timestamp_formatter( const log_timestamp_t* timestamp, char *out_buf, uint8_t size );
static void logger_writer_routine( void * arg );
static void update_timestamp( log_timestamp_t * );

/*
*	================================== Static Variables ==================================
*/

static atomic_bool s_context_mgr_initialized = false;
static atomic_bool s_context_mgr_active = false;
static atomic_bool s_context_mgr_stop_requested = false;
static pthread_cond_t s_context_mgr_cv = PTHREAD_COND_INITIALIZER;
static pthread_rwlock_t s_context_mgr_lock = PTHREAD_RWLOCK_INITIALIZER;
static logger_context_mgr_t s_context_mgr;
static task_params_t s_context_task_params = 
{
    .arg = &s_context_mgr,
    .can_join = true,
    .fn = logger_writer_routine,
    .prio = LOG_WRITTER_PRIO,
    .stack_size = 0xFFFF
};


/*
*	================================== Function Definitions ==================================
*/

static void logger_context_mgr_init(  )
{
    if( !s_context_mgr_initialized )
    {   
        // initialized contexts
        for( uint8_t i = 0; i < MAX_LOGGER_CONTEXTS; i++ )
        {
            WR_LOCK(s_context_mgr_lock)
            {
                s_context_mgr.contexts[i] = NULL;
            }   
            RW_UNLOCK(s_context_mgr_lock)
        }

        // Start writer thread
        s_context_mgr._log_writer = task_create( &s_context_task_params );
        task_start( s_context_mgr._log_writer );

        s_context_mgr_initialized = true;
    }
}

static void logger_context_mgr_deinit( )
{
    if( s_context_mgr_initialized )
    {
        // mark that a stop has been requested
        s_context_mgr_stop_requested = true;
        
        // Wait for the logger to stop
        while( s_context_mgr_active ) usleep(5000);
        
        for( uint8_t i = 0; i < MAX_LOGGER_CONTEXTS; i++ )
        {
            WR_LOCK(s_context_mgr_lock)
            {
                if( s_context_mgr.contexts[i] )
                {
                    free( s_context_mgr.contexts[i] );
                    s_context_mgr.contexts[i] = NULL;
                }
            }   
            RW_UNLOCK(s_context_mgr_lock)
        }
        
        task_stop( s_context_mgr._log_writer );
        task_destroy( s_context_mgr._log_writer );
        s_context_mgr._log_writer = NULL;
        
        s_context_mgr_initialized = false;
        s_context_mgr_stop_requested = false;
    }
}

/*====================================================================*/

static logger_ret_type logger_context_mgr_register_context( logger_t *logger, const logger_params_t *params )
{
    uint8_t i;
    logger_ret_type ret;
    
    WR_LOCK( s_context_mgr_lock )
    {
        // Find available slot and instantiate context
        for( i = 0; i < MAX_LOGGER_CONTEXTS; i++ )
        {
            if( s_context_mgr.contexts[i] == NULL ) break;
        }
        
        // instantiate if there's an available slot
        if( i < MAX_LOGGER_CONTEXTS )
        {
            logger->context_id = i;
            
            ret = logger_context_create( &(s_context_mgr.contexts[i]), params );

            if( ret != LOGGER_SUCCESS )
            {
                printf("logger_context_mgr_register_context(%u) %s\n",
                    i,
                    log_ret_type_str( ret)
                );
            }
        }
        else
        {
            ret = LOGGER_INIT_FAILED_CONTEXT_LIMIT_HIT;
        }        
    }
    RW_UNLOCK( s_context_mgr_lock )

    return ret;
}

static logger_ret_type logger_context_mgr_unregister_context( logger_t * logger  )
{
    uint8_t i;
    logger_context_t* context;

    WR_LOCK( s_context_mgr_lock )
    {
        i = logger->context_id;

        context = s_context_mgr.contexts[i];
        
        if( context )
        {
            // _LOCK( context->lock )
            {
                logger_context_destroy( &context );
            }
            // _UNLOCK( context->lock )
        }

        s_context_mgr.contexts[i] = NULL;
    }
    RW_UNLOCK( s_context_mgr_lock )

    logger->context_id = INVALID_LOGGER_CONTEXT_ID;

    return LOGGER_SUCCESS;
}

static logger_ret_type logger_context_mgr_append_log( logger_t * logger, log_entry_t * entry )
{
    uint8_t i;
    logger_context_t* context;

    RD_LOCK( s_context_mgr_lock )
    {
        i = logger->context_id;
        
        
        context = s_context_mgr.contexts[i];
        
        if( context )
        {
            logger_context_append_log( context, entry );
        }
    }
    RW_UNLOCK( s_context_mgr_lock )

    return LOGGER_SUCCESS;
}

static void logger_context_mgr_set_log_level( logger_t *logger, log_level level )
{
    uint8_t i;
    logger_context_t* context;

    RD_LOCK( s_context_mgr_lock )
    {
        i = logger->context_id;

        context = s_context_mgr.contexts[i];
        
        if( context )
        {
            logger_context_set_log_level(context, level);
        }
    }
    RW_UNLOCK( s_context_mgr_lock )

}

static const log_level logger_context_mgr_get_log_level( logger_t *logger )
{
    uint8_t i;
    log_level level = LOG_LEVEL_DEFAULT;
    logger_context_t* context;

    RD_LOCK( s_context_mgr_lock )
    {
        i = logger->context_id;

        context = s_context_mgr.contexts[i];
        
        if( context )
        {
            level = logger_context_get_log_level(context);
        }
    }
    RW_UNLOCK( s_context_mgr_lock )

    return level;
}

/*====================================================================*/

static logger_ret_type logger_context_create( logger_context_t** context_ptr, const  logger_params_t* params )
{
    (*context_ptr) = calloc(1, sizeof(logger_context_t));
    return logger_context_init( *context_ptr, params );
}


static logger_ret_type logger_context_init( logger_context_t* context, const logger_params_t* params )
{
    logger_ret_type ret;

    // context->lock
    context->_init = false;
    context->prior = params->prior;
    context->level = params->level;
    context->entry_count = 0;
    context->entry_buffer_cap = params->max_log_buf_enttry_cnt;
    context->max_file_size = params->max_file_size;
    context->timestamp_formatter = params->timestamp_formatter ? params->timestamp_formatter : default_timestamp_formatter;

    if( strlen(params->log_fname) <= LOG_FNAME_LEN )
    {
        strncpy( context->log_fname, params->log_fname, LOG_FNAME_LEN );
        
        if( strlen(params->log_fname_ext) <= LOG_FNAME_EXT_LEN )
        {
            strncpy( context->log_fname_ext, params->log_fname_ext, LOG_FNAME_EXT_LEN );
            
            if( strlen(params->log_path) <= LOG_PATH_LEN )
            {
                strncpy( context->log_path, params->log_path, LOG_FNAME_EXT_LEN );
                
                // pthread mutex
                pthread_mutex_init( &context->lock, NULL );
                
                // instantiate the entry queue
                context->entry_queue = sync_cir_buffer_create( true );
                
                context->_init = true;
                ret = LOGGER_SUCCESS;
            }
            else
            {
                ret = LOGGER_INIT_FAILED_EXCEEDED_PATH_LIMIT;
            }
        }
        else
        {
            ret = LOGGER_INIT_FAILED_EXCEEDED_FNAME_EXT_LIMIT;
        }
    }
    else
    {
        ret = LOGGER_INIT_FAILED_EXCEEDED_FNAME_LIMIT;
    }

    return ret;
}

static void logger_context_append_log( logger_context_t* context, log_entry_t * entry )
{
    _LOCK( context->lock )
    {
        sync_cir_buffer_write( context->entry_queue, (const char*)entry, sizeof(log_entry_t), false );
    }
    _UNLOCK( context->lock )
}

static bool logger_context_ready_to_write( logger_context_t* context )
{
    bool ret;

    _LOCK( context->lock )
    {
        ret = (sync_cir_buffer_is_empty( context->entry_queue ) == false);
    }
    _UNLOCK( context->lock )

    return ret;
}

static void logger_context_set_log_level( logger_context_t* context, const log_level level )
{
    // _LOCK( context->lock )
    {
        context->level = level;
    }
    // _UNLOCK( context->lock )
}

static log_level logger_context_get_log_level( logger_context_t* context)
{
    log_level ret;

    // _LOCK( context->lock )
    {
        ret = context->level;
    }
    // _UNLOCK( context->lock )

    return ret;
}


static bool logger_context_open_file(logger_context_t *context)
{
    file_io_ret_type res = true;
    bool ret = true;

    if( context->file.opened == false )
    {
        log_timestamp_t timestamp;
        char timestamp_str[50];
        char filename_with_ext[LOG_FNAME_LEN];

        update_timestamp( &timestamp );
        context->timestamp_formatter( &timestamp, timestamp_str, 50 );

        snprintf( filename_with_ext, LOG_FNAME_LEN, "%s_%s.%s",
            context->log_fname,
            timestamp_str,
            context->log_fname_ext
        );
        
        res = file_init( &context->file, FILE_T_ASCII, FILE_OP_MASK_APPEND, context->log_path, filename_with_ext );
        if( res != FILE_IO_RET_TYPE__SUCCESS )
        {
            printf("File Intialization failed : %s\n", file_io_ret_str(res) );
            ret = false;
        }
        
        res = file_open( &(context->file) );
        if( res != FILE_IO_RET_TYPE__SUCCESS )
        {
            printf("File open failed : %s\n", file_io_ret_str(res) );
            ret = false;
        }
    }

    return ret;
}

static bool logger_context_close_file(logger_context_t *context)
{
    bool ret = true;
    file_io_ret_type res = true;

    if( context->file.opened )
    {
        res = file_close( &context->file );
       
        if( res != FILE_IO_RET_TYPE__SUCCESS )
        {
            printf("File close failed : %s\n", file_io_ret_str(res) );
            ret = false;
        }

        res = file_deinit( &context->file );

        if( res != FILE_IO_RET_TYPE__SUCCESS )
        {
            printf("File deinit failed : %s\n", file_io_ret_str(res) );
            ret = false;
        }
    }

    return ret;
}


static void logger_context_write_log( logger_context_t* context )
{
    log_entry_t entry;
    char write_buffer[ LOG_ENTRY_STR_LEN];

    file_t * file;

    _LOCK( context->lock )
    {
        sync_cir_buffer_read( context->entry_queue, (char*)&entry, sizeof(entry), false );

        file = &context->file;

        if( file )
        {
            if( file->opened )
            {
                if( file->chars.wr >= context->max_file_size )
                {
                    if(logger_context_close_file(context) == false)
                    {
                        exit(-1);
                    }

                    if(logger_context_open_file(context) == false)
                    {
                        exit(-1);
                    }
                }
            }
            else
            {
                // open for writing
                if(logger_context_open_file(context) == false)
                {
                    exit(-1);
                }
            }

            // write to the file
            log_entry_to_str( &entry, write_buffer, LOG_ENTRY_STR_LEN );
            file_write_ascii( file, write_buffer, strlen(write_buffer) );
        }
        else
        {

        }
        
    }
    _UNLOCK( context->lock )
}

static void logger_context_flush_logs( logger_context_t* context )
{
    file_t * file;

    _LOCK( context->lock )
    {
        file = &context->file;

        if( file )
        {
            file_flush( file );
        }
    }
    _UNLOCK( context->lock )
}


static logger_ret_type logger_context_deinit( logger_context_t* context )
{
    if( context->_init )
    {
        _LOCK( context->lock );
        {
            if( context->entry_queue )
            {
                sync_cir_buffer_destroy( context->entry_queue );
                context->entry_queue = NULL;
            }

            // Close the file
            file_deinit(&context->file);

            context->timestamp_formatter = NULL;
            context->_init = false;
        }
        _UNLOCK( context->lock );

        pthread_mutex_destroy( &context->lock );
    }

    return LOGGER_SUCCESS;
}

static logger_ret_type logger_context_destroy( logger_context_t**context_ptr )
{
    if( *context_ptr )
    {
        logger_context_deinit( *context_ptr );
    }
    
    return LOGGER_SUCCESS;
}

/*====================================================================*/

static const char* log_level_to_str( const log_level level)
{
    switch (level)
    {
        case LOG_LEVEL_TRACE: return "Trace";
        case LOG_LEVEL_DEBUG: return "Debug";
        case LOG_LEVEL_INFO: return "Info";
        case LOG_LEVEL_ERROR: return "Error";
        case LOG_LEVEL_FATAL: return "Fatal";
        default: return "-";
    }
}

static uint32_t log_entry_to_str( const log_entry_t * entry, char *out_buf, uint32_t size )
{
    char timestamp_str[50];
    default_timestamp_formatter( &entry->timestamp, timestamp_str, sizeof(timestamp_str) );

    int ret = snprintf( out_buf, size, "[%s] %s : %s\n",
        timestamp_str,
        log_level_to_str( entry->level ),
        entry->msg
    );

    if( ret < 0 ) ret = 0;

    return (uint32_t)ret;
}

/*====================================================================*/


static uint8_t default_timestamp_formatter( const log_timestamp_t* timestamp, char *out_buf, uint8_t size )
{
    snprintf( out_buf, size, "%llu", timestamp->raw);
    return strlen(out_buf);
}


static void logger_writer_routine( void * arg )
{
    log_entry_t log;
    logger_context_t* c_ctxt;
    logger_context_mgr_t* mgr = (logger_context_mgr_t*) arg;

    s_context_mgr_active = true;
    
    // No stop has been requested
    while( !s_context_mgr_stop_requested )
    {
        // loop every 5 ms
        usleep(5000);

        // iterate over the contexts and check if there's data available
        for( uint8_t i = 0; i < MAX_LOGGER_CONTEXTS; i++ )
        {
            RD_LOCK( s_context_mgr_lock )
            {
                c_ctxt = s_context_mgr.contexts[i];
                
                if( c_ctxt )
                {
                    while( logger_context_ready_to_write(c_ctxt) )
                    {
                        logger_context_write_log( c_ctxt );
                    }
                    logger_context_flush_logs( c_ctxt );
                }
            }
            RW_UNLOCK( s_context_mgr_lock )
            
        }
    }

    s_context_mgr_active = false;

}

static void update_timestamp( log_timestamp_t * timestamp )
{
    struct timespec spec;
    static const uint64_t s_div = 1000000000;
    clock_gettime( CLOCK_MONOTONIC, &spec );
    
    uint64_t sec = spec.tv_sec;
    uint64_t nsec = spec.tv_nsec;
    
    timestamp->raw = sec / s_div;
    timestamp->raw += nsec % s_div;
}

/*====================================================================*/


void logger_init( )
{
    logger_context_mgr_init();
}

void logger_deinit( )
{
    logger_context_mgr_deinit();
}


logger_ret_type logger_get_instance( logger_t *logger, const logger_params_t* params )
{
    return logger_context_mgr_register_context( logger, params );
}

// logger_ret_type logger_log( logger_t* logger, const log_level level, const char * msg )
// {

// }

void log_set_level( logger_t* logger, log_level  level)
{
    logger_context_mgr_set_log_level( logger, level );
}

log_level log_get_level( logger_t* logger )
{
    return logger_context_mgr_get_log_level( logger);
}

const char * log_ret_type_str( const logger_ret_type type )
{
    switch(type)
    {
        case LOGGER_SUCCESS: return "Success";
        case LOGGER_INIT_FAILED_CONTEXT_LIMIT_HIT: return "Context count limit hit";
        case LOGGER_INIT_FAILED_EXCEEDED_FNAME_LIMIT: return "Filename size limit exceeded";
        case LOGGER_INIT_FAILED_EXCEEDED_FNAME_EXT_LIMIT: return "Filename extension size limit exceeded";
        case LOGGER_INIT_FAILED_EXCEEDED_PATH_LIMIT: return "Path Limit exeeded";
        case LOGGER_INIT_FAILED: 
        default:
            return "Unknown failure";
    }
}

logger_ret_type log_trace( logger_t* logger, const char * msg, ... )
{
    log_entry_t entry;

    if( log_get_level(logger) <= LOG_LEVEL_TRACE )
    {
        va_list args;
        va_start( args, msg );
        vsnprintf( entry.msg, LOG_MSG_LEN, msg, args );
        va_end( args );
    
        entry.level = LOG_LEVEL_TRACE;
        update_timestamp( &entry.timestamp );
    
        logger_context_mgr_append_log( logger, &entry );
    }


    return LOGGER_SUCCESS;
}

logger_ret_type log_debug( logger_t* logger, const char * msg, ... )
{
    log_entry_t entry;

    if( log_get_level(logger) <= LOG_LEVEL_DEBUG )
    {
        va_list args;
        va_start( args, msg );
        vsnprintf( entry.msg, LOG_MSG_LEN, msg, args );
        va_end( args );
        
        entry.level = LOG_LEVEL_DEBUG;
        update_timestamp( &entry.timestamp );
    
        logger_context_mgr_append_log( logger, &entry );
    }

    return LOGGER_SUCCESS;
}

logger_ret_type log_info( logger_t* logger, const char * msg, ... )
{
    log_entry_t entry;

    if( log_get_level(logger) <= LOG_LEVEL_INFO )
    {
        va_list args;
        va_start( args, msg );
        vsnprintf( entry.msg, LOG_MSG_LEN, msg, args );
        va_end( args );
        
        entry.level = LOG_LEVEL_INFO;
        update_timestamp( &entry.timestamp );
    
        logger_context_mgr_append_log( logger, &entry );
    }

    return LOGGER_SUCCESS;
}

logger_ret_type log_error( logger_t* logger, const char * msg, ... )
{
    log_entry_t entry;

    if( log_get_level(logger) <= LOG_LEVEL_ERROR )
    {
        va_list args;
        va_start( args, msg );
        vsnprintf( entry.msg, LOG_MSG_LEN, msg, args );
        va_end( args );
        
        entry.level = LOG_LEVEL_ERROR;
        update_timestamp( &entry.timestamp );
    
        logger_context_mgr_append_log( logger, &entry );
    }

    return LOGGER_SUCCESS;
}

logger_ret_type log_fatal( logger_t* logger, const char * msg, ... )
{
    log_entry_t entry;

    if( log_get_level(logger) <= LOG_LEVEL_FATAL )
    {
        va_list args;
        va_start( args, msg );
        vsnprintf( entry.msg, LOG_MSG_LEN, msg, args );
        va_end( args );
        
        entry.level = LOG_LEVEL_FATAL;
        update_timestamp( &entry.timestamp );
    
        logger_context_mgr_append_log( logger, &entry );
    }

    return LOGGER_SUCCESS;
}

logger_ret_type logger_release_instance( logger_t* logger )
{
    return logger_context_mgr_unregister_context( logger );
}

