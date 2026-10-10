#if !defined(__FILE_IO_TESTER_H__)
#define __FILE_IO_TESTER_H__

#include "file_io.h"
#include <assert.h>
#include <string.h>
#include <errno.h>

// static const char *filepath = "/Users/chimaochiagha/Projects/DNS/code-crafters-dns";
static const char *filepath = "";
static const char *filename = "fileio_testfile.txt";


static const char * to_write[] = 
{
    "Line 1",
    "Line 2",
    "Line 3"
};

static char read_buf[10];

static file_t *file;

static bool file_io_test_ascii();
static bool file_io_test_ascii_open( uint8_t ops );
static bool file_io_test_ascii_close();
static bool file_io_test_ascii_write();
static bool file_io_test_ascii_read();
static bool file_io_test_ascii_write_lines();
static bool file_io_test_ascii_read_lines();

void run_tests_file_io();


void run_tests_file_io()
{
    file_io_test_ascii();
}

static bool file_io_test_ascii()
{
    // test File open and close
    assert( file_io_test_ascii_open(FILE_OP_MASK_WRITE) );
    assert( file_io_test_ascii_close() );
    
    // Test open write and close
    assert( file_io_test_ascii_open((FILE_OP_MASK_APPEND)) );
    assert( file_io_test_ascii_write() );
    assert( file_io_test_ascii_write_lines() );
    assert( file_io_test_ascii_close() );
    
    // // Test open write many and close
    assert( file_io_test_ascii_open((FILE_OP_MASK_READ)) );
    assert( file_io_test_ascii_read() );
    assert( file_io_test_ascii_read_lines() );
    assert( file_io_test_ascii_close() );

    return true;
}

static bool file_io_test_ascii_open( uint8_t ops )
{
    file_io_ret_type ret = file_create( &file, FILE_T_ASCII, ops, filepath, filename );

    if( ret == FILE_IO_RET_TYPE__SUCCESS )
    {
        if( (ret = file_open( file )) == FILE_IO_RET_TYPE__SUCCESS )
        {
            assert( file->file_ptr );
            assert( file->opened );
            assert( file->chars.rd == 0 );
            assert( file->chars.wr == 0 );
            assert( file->lines.rd == 0 );
            assert( file->lines.wr == 0 );
            assert( file->_type == FILE_T_ASCII );
            assert( (file->_op_mask & ops) == ops);
            assert( strcmp(file->_name, filename) == 0 );
            assert( strcmp(file->_path, filepath) == 0 );
        }
        else
        {
            printf("file_io_test_ascii_open() failed - '%s'\n", file_io_ret_str( ret ));
        }
    }
    else
    {
        printf("file_io_test_ascii_open() failed - '%s'\n", file_io_ret_str( ret ));
    }


    return ret == FILE_IO_RET_TYPE__SUCCESS;
}

static bool file_io_test_ascii_close()
{
    file_io_ret_type ret = file_release( &file );

    if( ret == FILE_IO_RET_TYPE__SUCCESS )
    {
        assert( file == NULL );
    }
    else
    {
        printf("file_io_test_ascii_open() failed - '%s'\n", file_io_ret_str( ret ));
    }

    return ret == FILE_IO_RET_TYPE__SUCCESS;
}

static bool file_io_test_ascii_write()
{
    file_io_ret_type ret = file_write_ascii( file, to_write[0], strlen(to_write[0]) );

    if( ret != FILE_IO_RET_TYPE__SUCCESS )
    {
        printf("file_io_test_ascii_write() failed - '%s'\n", file_io_ret_str( ret ));
        printf("SysError - %s\n", strerror(errno) );
    }

    return (ret == FILE_IO_RET_TYPE__SUCCESS);
}

static bool file_io_test_ascii_write_lines()
{
    for( int i = 0; i < 3; i++ )
    {
        if(file_write_ascii( file, to_write[i], strlen(to_write[i]) ) != FILE_IO_RET_TYPE__SUCCESS )
        {
            return false;
        }
    }

    return true;
}

static bool file_io_test_ascii_read()
{
    read_buf[0] = 0;
    file_io_ret_type ret = file_read_ascii( file, read_buf, strlen(to_write[0]) + 1 );

    if( ret == FILE_IO_RET_TYPE__SUCCESS )
    {
        if( strcmp(read_buf, to_write[0]) )
        {
            printf("Read mismatch - %s != %s\n", read_buf, to_write[0] );
            return false;
        }
    }
    else
    {
        printf("file_io_test_ascii_read(%s) failed - '%s'\n", read_buf, file_io_ret_str( ret ));
        printf("SysError - %s\n", strerror(errno) );
    }

    return (ret == FILE_IO_RET_TYPE__SUCCESS);
}

static bool file_io_test_ascii_read_lines()
{
    for( int i = 0; i < 3; i++ )
    {
        if(file_read_ascii( file, read_buf, strlen(to_write[i]) + 1 ) == FILE_IO_RET_TYPE__SUCCESS )
        {
            if (strcmp( read_buf, to_write[i] ) ) 
            {
                printf("Read mismatch - %s != %s\n", read_buf, to_write[i] );
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}


#endif // __FILE_IO_TESTER_H__
