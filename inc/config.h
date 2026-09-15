#if !defined(__CONFIG_H__)
#define __CONFIG_H__

/**
 * Structures for managing process and server configurations
 */

#include "common.h"

/// @brief System level conlfigurations
typedef struct process_config
{
    uint8_t log_level; // server log level
} process_config_t;

#endif // __CONFIG_H__
