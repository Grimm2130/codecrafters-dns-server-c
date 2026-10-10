#include "file_io.h"
#include "string.h"
#include <stdlib.h>

#define FILE_READ_ONLY_MASK         (FILE_OP_MASK_READ)
#define FILE_WRITE_ONLY_MASK        (FILE_OP_MASK_WRITE)
#define FILE_APPEND_ONLY_MASK       (FILE_OP_MASK_APPEND)
#define FILE_READ_WRITE_MASK        (FILE_OP_MASK_READ | FILE_OP_MASK_WRITE)
#define FILE_APPEND_READ_MASK       (FILE_OP_MASK_APPEND | FILE_OP_MASK_READ)

file_io_ret_type file_init( file_t *fp, file_type type, uint8_t mask, const char *path, const char *name )
{
    file_io_ret_type ret;

    if( path )
    {
        if( strlen(path) <= FILE_PATH_MAX )
        {
            strncpy(fp->_path, path, FILE_PATH_MAX );
            
            if( name )
            {
                if( strlen(name) <= FILE_NAME_MAX )
                {
                    strncpy(fp->_name, name, FILE_NAME_MAX );

                    if( mask & (uint8_t)(FILE_OP_MASK_READ | FILE_OP_MASK_WRITE | FILE_OP_MASK_APPEND) )
                    {
                        fp->_op_mask = mask;

                        if( (type == FILE_T_ASCII) || (type == FILE_T_BINARY) )
                        {
                            fp->_type = type;

                            fp->opened = false;
                            fp->chars.rd = fp->chars.wr = 0;
                            fp->lines.rd = fp->lines.wr = 0;
                            fp->file_ptr = NULL;

                            ret = FILE_IO_RET_TYPE__SUCCESS;
                        }
                        else
                        {
                            ret = FILE_IO_RET_TYPE__FAIL_ON_INIT_INV_F_TYPE;
                        }
                    }
                    else
                    {
                        ret = FILE_IO_RET_TYPE__FAIL_ON_INIT_INV_OP_MASK;
                    }
                }
                else
                {
                    ret = FILE_IO_RET_TYPE__FAIL_ON_INIT_NAME_TOO_LARGE;
                }
            }
            else
            {
                ret = FILE_IO_RET_TYPE__FAIL_ON_INIT_NO_NAME;
            }
        }
        else
        {
            ret = FILE_IO_RET_TYPE__FAIL_ON_INIT_PATH_TOO_LARGE;
        }
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_INIT_NO_PATH;
    }

    return ret;
}

file_io_ret_type file_create( file_t **fpp, file_type type, uint8_t mask, const char *path, const char *name )
{
    file_io_ret_type ret;

    if( fpp )
    {
        (*fpp) = calloc(1, sizeof(file_t));
        ret = file_init( *fpp, type, mask, path, name );
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR;
    }

    return ret;
}

file_io_ret_type file_open( file_t *fp)
{
    file_io_ret_type ret;

    if( fp )
    {
        char *mode;

        // determine mode
        switch (fp->_type)
        {
            case FILE_T_ASCII:
            case FILE_T_BINARY:
            {
                if((fp->_op_mask & FILE_READ_WRITE_MASK) == FILE_READ_WRITE_MASK) mode = "w+";
                else if((fp->_op_mask & FILE_APPEND_READ_MASK) == FILE_APPEND_READ_MASK) mode = "a+";
                else if((fp->_op_mask & FILE_READ_ONLY_MASK) == FILE_READ_ONLY_MASK) mode = "r";
                else if((fp->_op_mask & FILE_WRITE_ONLY_MASK) == FILE_WRITE_ONLY_MASK) mode = "w";
                else if((fp->_op_mask & FILE_APPEND_ONLY_MASK) == FILE_APPEND_ONLY_MASK) mode = "a";
                else mode = NULL;
            }
            break;
            
            default:
                mode = NULL;
            break;
        }

        if( mode )
        {
            int len;
            char full_path[FILE_PATH_MAX + FILE_NAME_MAX ];
            
            if( strlen( fp->_path ) )
            {
                len = snprintf( full_path, FILE_PATH_MAX + FILE_NAME_MAX, "%s/%s", fp->_path, fp->_name );
            }
            else
            {
                len = snprintf( full_path, FILE_PATH_MAX + FILE_NAME_MAX, "%s", fp->_name );
            }
            
            printf("Attempting to open(%s) %s\n", mode, full_path);

            if( len > 0 )
            {
                fp->file_ptr = fopen( full_path, mode );

                if( fp->file_ptr )
                {
                    fp->opened = true;
                    ret = FILE_IO_RET_TYPE__SUCCESS;
                }
                else
                {
                    ret = FILE_IO_RET_TYPE__FAIL_ON_OPEN;
                }
            }
            else if( len == 0 )
            {
                // TODO
                ret = FILE_IO_RET_TYPE__FAIL_UNKNOWN;
            }
            else
            {
                // TODO
                ret = FILE_IO_RET_TYPE__FAIL_UNKNOWN;
            }
        }
        else
        {
            ret = FILE_IO_RET_TYPE__FAIL_ON_INIT_INV_F_TYPE;
        }
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR;
    }

    return ret;
}

file_io_ret_type file_read_ascii( file_t *fp, char * out_buf, uint32_t n_count )
{
    file_io_ret_type ret;

    if( fp )
    {
        if( fp->opened )
        {
            if( fgets( out_buf, n_count, fp->file_ptr ) )
            {
                fp->chars.rd += strlen(out_buf);
                fp->lines.rd++;
                ret = FILE_IO_RET_TYPE__SUCCESS;
            }
            else
            {
                ret=  FILE_IO_RET_TYPE__FAIL_ON_READ_EMPTY;
            }
        }
        else
        {
            ret = FILE_IO_RET_TYPE__FAIL_ON_READ_NOT_OPEN;
        }
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR;
    }

    return ret;
}

file_io_ret_type file_read_binary( file_t *fp, char * out_buf, uint32_t i_size, uint32_t *n_count )
{
    file_io_ret_type ret;

    if( fp )
    {
        if( fp->opened )
        {
            size_t count;

            if( (count = fread( out_buf, i_size, *n_count, fp->file_ptr )) )
            {
                *n_count = count;
                ret = FILE_IO_RET_TYPE__SUCCESS;
            }
            else
            {
                ret=  FILE_IO_RET_TYPE__FAIL_ON_READ_EMPTY;
            }
        }
        else
        {
            ret = FILE_IO_RET_TYPE__FAIL_ON_READ_NOT_OPEN;
        }
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR;
    }

    return ret;
}


file_io_ret_type file_write_ascii( file_t *fp, const char * in_buf, uint32_t n_count )
{
    file_io_ret_type ret;

    if( fp )
    {
        if( fp->opened )
        {
            if( fputs( in_buf, fp->file_ptr ) >= 0 )
            {
                fp->chars.wr += strlen(in_buf);
                ret = FILE_IO_RET_TYPE__SUCCESS;
            }
            else
            {
                ret = FILE_IO_RET_TYPE__FAIL_ON_WRITE_FAIL;
            }
        }
        else
        {
            ret=  FILE_IO_RET_TYPE__FAIL_ON_WRITE_NOT_OPEN;
        }
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR;
    }

    return ret;
}

file_io_ret_type file_write_binary( file_t *fp, const char *in_buf, uint32_t i_size, uint32_t *n_count )
{
    file_io_ret_type ret;

    if( fp )
    {
        if( fp->opened )
        {
            size_t count;

            if( (count = fwrite( in_buf, i_size, *n_count, fp->file_ptr )) )
            {
                fp->chars.wr += strlen(in_buf);
                *n_count = count;
                ret = FILE_IO_RET_TYPE__SUCCESS;
            }
            else
            {
                ret=  FILE_IO_RET_TYPE__FAIL_ON_WRITE_NOT_OPEN;
            }
        }
        else
        {
            ret = FILE_IO_RET_TYPE__FAIL_ON_WRITE_EMPTY;
        }
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR;
    }

    return ret;
}

file_io_ret_type file_close( file_t *fp)
{
    file_io_ret_type ret;

    if( fp )
    {
        if( fp->opened )
        {
            fclose(fp->file_ptr);
            fp->file_ptr = NULL;
            fp->opened = false;
        }

        ret = FILE_IO_RET_TYPE__SUCCESS;
    }
    else
    {
        ret = FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR;
    }

    return ret;
}

file_io_ret_type file_deinit( file_t *fp)
{
    if( fp )
    {
        file_close( fp );
    }

    return FILE_IO_RET_TYPE__SUCCESS;
}

file_io_ret_type file_release( file_t **fpp)
{
    if( fpp )
    {
        file_deinit( *fpp );
        free( *fpp );    
        (*fpp) = NULL;
    }

    return FILE_IO_RET_TYPE__SUCCESS;
}

const char *file_io_ret_str( file_io_ret_type type )
{
    switch (type)
    {
    case FILE_IO_RET_TYPE__SUCCESS: return "Success";
    case FILE_IO_RET_TYPE__FAIL_ON_NULL_FILE_PTR: return "Null File Pointer";
    case FILE_IO_RET_TYPE__FAIL_ON_INIT_NO_PATH: return "No Path";
    case FILE_IO_RET_TYPE__FAIL_ON_INIT_PATH_TOO_LARGE: return "Path Too Large";
    case FILE_IO_RET_TYPE__FAIL_ON_INIT_NO_NAME: return "No Filename";
    case FILE_IO_RET_TYPE__FAIL_ON_INIT_NAME_TOO_LARGE: return "Filename too large";
    case FILE_IO_RET_TYPE__FAIL_ON_INIT_INV_F_TYPE: return "Invalid file type";
    case FILE_IO_RET_TYPE__FAIL_ON_INIT_INV_OP_MASK: return "Invalid file operation mask";
    case FILE_IO_RET_TYPE__FAIL_ON_OPEN: return "Fail on Open";
    case FILE_IO_RET_TYPE__FAIL_ON_CLOSE: return "Fail on Close";
    case FILE_IO_RET_TYPE__FAIL_ON_READ_EMPTY: return "Empty Read";
    case FILE_IO_RET_TYPE__FAIL_ON_READ_NOT_OPEN: return "File not open";
    case FILE_IO_RET_TYPE__FAIL_ON_WRITE_EMPTY: return "Empty Write";
    case FILE_IO_RET_TYPE__FAIL_ON_WRITE_FAIL: return "Failed Write";
    case FILE_IO_RET_TYPE__FAIL_ON_WRITE_NOT_OPEN: return "File Not Open";
    case FILE_IO_RET_TYPE__FAIL_UNKNOWN:
    default:
    return "File Failure Unknown";
    }
}