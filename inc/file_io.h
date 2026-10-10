#if !defined(__FILE_IO_H__)
#define __FILE_IO_H__

/**
 * @brief File I/O Functionality
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define FILE_PATH_MAX   1024U
#define FILE_NAME_MAX   255U

/// @brief File Type
typedef enum _file_type
{
    FILE_T_ASCII,
    FILE_T_BINARY
} file_type;

/// @brief File Operation mask
typedef enum _file_op_mask
{
    FILE_OP_MASK_READ    = 1,
    FILE_OP_MASK_WRITE   = 2,
    FILE_OP_MASK_APPEND  = 4
} file_op_mask;

/// @brief File IO Return value
typedef enum _file_io_ret_type
{
    FILE_IO_RET_TYPE__SUCCESS,
    FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR,        // Null File pointer
    FILE_IO_RET_TYPE__FAIL_ON_INIT_NO_PATH,         // no path
    FILE_IO_RET_TYPE__FAIL_ON_INIT_PATH_TOO_LARGE,  // file path too large
    FILE_IO_RET_TYPE__FAIL_ON_INIT_NO_NAME,         // no name
    FILE_IO_RET_TYPE__FAIL_ON_INIT_NAME_TOO_LARGE,  // File name too large
    FILE_IO_RET_TYPE__FAIL_ON_INIT_INV_F_TYPE,      // invalid file type
    FILE_IO_RET_TYPE__FAIL_ON_INIT_INV_OP_MASK,     // invalid file op
    FILE_IO_RET_TYPE__FAIL_ON_OPEN,
    FILE_IO_RET_TYPE__FAIL_ON_CLOSE,
    FILE_IO_RET_TYPE__FAIL_ON_READ_EMPTY,
    FILE_IO_RET_TYPE__FAIL_ON_READ_NOT_OPEN,
    FILE_IO_RET_TYPE__FAIL_ON_WRITE_EMPTY,
    FILE_IO_RET_TYPE__FAIL_ON_WRITE_FAIL,
    FILE_IO_RET_TYPE__FAIL_ON_WRITE_NOT_OPEN,
    FILE_IO_RET_TYPE__FAIL_UNKNOWN,
} file_io_ret_type;

/// @brief File
typedef struct file
{
    // Flag to indicate that the file is opened
    bool opened;

    // File operation mask
    uint8_t _op_mask;

    // File Type
    file_type _type;
    
    // Characters
    struct
    {
        uint32_t rd;    // read
        uint32_t wr;    // written
    } chars;
    
    /// @brief Lines
    struct
    {
        uint32_t rd;    // read
        uint32_t wr;    // written
    } lines;

    char _path[FILE_PATH_MAX];
    char _name[FILE_NAME_MAX];

    FILE * file_ptr;
} file_t;


file_io_ret_type file_init( file_t *, file_type, uint8_t, const char *, const char *);
file_io_ret_type file_create( file_t **, file_type, uint8_t, const char *, const char *);
file_io_ret_type file_open( file_t *);
file_io_ret_type file_read_ascii( file_t *, char *, uint32_t );
file_io_ret_type file_read_binary( file_t *, char *, uint32_t, uint32_t*);
file_io_ret_type file_write_ascii( file_t *, const char *, uint32_t );
file_io_ret_type file_write_binary( file_t *, const char *, uint32_t, uint32_t* );
file_io_ret_type file_close( file_t *);
file_io_ret_type file_deinit( file_t *);
file_io_ret_type file_release( file_t **);
const char *file_io_ret_str( file_io_ret_type );

#endif // __FILE_IO_H__
