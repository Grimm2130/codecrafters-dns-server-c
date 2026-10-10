#if !defined(__LOGGER_H__)
#define __LOGGER_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct log_timestamp log_timestamp_t;

typedef uint8_t (*logger_time_stamp_formatter)(const log_timestamp_t *, char *out_buf, uint8_t size );

typedef enum _logger_ret_type
{
    LOGGER_SUCCESS,
    LOGGER_INIT_FAILED_CONTEXT_LIMIT_HIT,
    LOGGER_INIT_FAILED_EXCEEDED_FNAME_LIMIT,
    LOGGER_INIT_FAILED_EXCEEDED_FNAME_EXT_LIMIT,
    LOGGER_INIT_FAILED_EXCEEDED_PATH_LIMIT,
    LOGGER_INIT_FAILED_
} logger_ret_type;

/// @brief Log level
typedef enum _log_level
{
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_DEFAULT
} log_level;


/// @brief Logger instance ID
typedef struct logger
{
    uint8_t context_id;
} logger_t;

typedef struct logger_params
{
    uint8_t prior;
    log_level level;
    uint16_t max_log_buf_enttry_cnt;        // max count for logger entries
    uint32_t max_file_size;                 // max file size in bytes
    const char *log_fname_ext;              // Logger filename ext
    const char *log_path;                   // Logger file path
    const char *log_fname;                  // Logger File name
    logger_time_stamp_formatter timestamp_formatter;
} logger_params_t;

void logger_init( );
void logger_deinit( );
logger_ret_type logger_get_instance( logger_t *, logger_params_t* );
// logger_ret_type logger_log( logger_t*, const log_level level, const char * msg );
void log_set_level( logger_t*, log_level );
log_level log_get_level( logger_t* );
logger_ret_type log_trace( logger_t*, const char * msg, ... );
logger_ret_type log_debug( logger_t*, const char * msg, ... );
logger_ret_type log_info( logger_t*, const char * msg, ... );
logger_ret_type log_error( logger_t*, const char * msg, ... );
logger_ret_type log_fatal( logger_t*, const char * msg, ... );
logger_ret_type logger_release_instance( logger_t * );

#endif // __LOGGER_H__
