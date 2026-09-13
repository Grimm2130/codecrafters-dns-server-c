#include "cir_buffer.h"
#include "util.h"
#include "common.h"

cir_buffer_t* cir_buffer_create( const bool allow_overwrite )
{
    cir_buffer_t* cbuf = calloc(1, sizeof(cir_buffer_t));

    cir_buffer_create_s( cbuf, allow_overwrite );

    return cbuf;
}

void cir_buffer_create_s( cir_buffer_t* cbuf, const bool allow_overwrite)
{
    cbuf->allow_overwrite = allow_overwrite;
    cbuf->front = cbuf->rear = cbuf->size = 0;

    for( int i = 0; i < CIR_BUFFER_ENTRY_CAPACITY; i++ )
    {
        cbuf->buffer[i] = NULL;
    }
}

bool cir_buffer_is_empty( cir_buffer_t* cbuf)
{
    return cbuf->size == 0;
}

bool cir_buffer_is_full( cir_buffer_t* cbuf)
{
    return cbuf->size == CIR_BUFFER_ENTRY_CAPACITY;
}

/// @brief Read from the circular buffer
/// @param cbuf Circular buffer object pointer
/// @param buf Buffer to write to
/// @param amnt Amount to be written
/// @return Number of bytes read successfully
int cir_buffer_read( cir_buffer_t* cbuf, char *buf, const uint32_t amnt )
{   
    bool can_read = false;
    
    if( cir_buffer_is_empty( cbuf ) == false )
    {
        can_read = true;
    }

    if( can_read )
    {
        cbuf->size--;
        cbuf->buffer[cbuf->front] = calloc( amnt, sizeof(char) );
        strncpy( buf, cbuf->buffer[ cbuf->front], amnt );
        cbuf->front = (cbuf->front + 1) % CIR_BUFFER_ENTRY_CAPACITY;
    }

    return amnt;
}

/// @brief Circular Buffer Write: Write to a circular buffer
/// @param cbuf Circular buffer object pointer
/// @param buf Buffer to read from
/// @param amnt Amount to read from buffer
/// @return number of bytes written
int cir_buffer_write( cir_buffer_t* cbuf, const char * buf, const uint32_t amnt )
{
    bool can_write = false;
    
    if( cir_buffer_is_full( cbuf ) == false )
    {
        can_write = true;
    }

    if( can_write )
    {
        cbuf->size++;
        cbuf->buffer[cbuf->rear] = calloc( amnt, sizeof(char) );
        strncpy( cbuf->buffer[ cbuf->rear], buf, amnt );
        cbuf->rear = (cbuf->rear + 1) % CIR_BUFFER_ENTRY_CAPACITY;
    }

    return amnt;
}
