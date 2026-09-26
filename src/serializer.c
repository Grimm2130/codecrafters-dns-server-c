#include "serializer.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

#define SERIALIZER_DEFAULT_CAPACITY 1048576U

#define SERIALIZER_AVAILABLE_SIZE(srz_ptr)      ( ((srz_ptr)->cap - (srz_ptr)->size) )

static void buffer_shift( uint8_t *buf, uint32_t src_idx, uint32_t dst_idx, const uint32_t size );

serializer_t * serializer_create( )
{
    serializer_t * srz = calloc(1, sizeof(serializer_t));
    serializer_init( srz );
    return srz;
}

void serializer_init( serializer_t *srz )
{
    srz->size = 0;
    srz->cap = SERIALIZER_DEFAULT_CAPACITY;
    srz->buf = calloc( SERIALIZER_DEFAULT_CAPACITY, sizeof(uint8_t));
}

int serializer_append( serializer_t *srz, const char* data, int len )
{
    int ret = 1;

    if( len <= SERIALIZER_AVAILABLE_SIZE(srz) )
    {
        ret = 0;
        memcpy( (srz->buf + srz->size), data, len );
        srz->size += len;
    }

    return ret;
}

int serializer_insert( serializer_t *srz, const uint32_t start_idx, const char* data, int len )
{
    int ret = 1;

    if( len <= SERIALIZER_AVAILABLE_SIZE(srz) )
    {
        ret = 0;
        buffer_shift( srz->buf, start_idx, start_idx + len, srz->size - start_idx );
        memcpy( (srz->buf + start_idx), data, len );
        srz->size += len;
    }

    return ret;
}

int serializer_remove( serializer_t *srz, const uint32_t start_idx, char* data, int len )
{
    int ret = 1;
    const int end_idx = (start_idx + len);
    const int last_len = srz->size - end_idx;

    if( srz->size >= end_idx )
    {
        ret = 0;
        // Copy data into buffer
        memcpy( data, srz->buf + start_idx, len );
        // remove data from buffer
        memcpy( srz->buf + start_idx, srz->buf + end_idx, last_len );
        srz->size -= len;
    }

    return ret;
}

int serializer_copy( serializer_t *srz, const uint32_t start_idx, char* data, int len )
{
    int ret = 1;

    if( srz->size >= (start_idx + len) )
    {
        ret = 0;

        // Copy data into buffer
        memcpy( data, srz->buf + start_idx, len );
    }

    return ret;
}

const char* serializer_dump( serializer_t *srz )
{
    #define SERIALIZER_DUMP_BUFFER_SIZE     (SERIALIZER_DEFAULT_CAPACITY * 4)
    static char buf[SERIALIZER_DUMP_BUFFER_SIZE];
    int len = 0;

    for( uint32_t i = 0; i < srz->size; i++ )
    {
        len += snprintf( buf + len, SERIALIZER_DUMP_BUFFER_SIZE - len, "0x%.2x ", srz->buf[i] );
    }
    buf[len] = '\0';

    return buf;
}

void serializer_deinit( serializer_t *srz )
{
    if( srz->buf ) free( srz->buf );
    srz->size = srz->cap = 0;
}

void serializer_release( serializer_t *srz )
{
    serializer_deinit( srz );
    free( srz );
}

/// @brief Shift the serializer content from the start to the end location
/// @param srz Serializer object
/// @param src_idx Source index
/// @param dst_idx Destination index
/// @param size Size
static void buffer_shift( uint8_t *buf, uint32_t src_idx, uint32_t dst_idx, const uint32_t size )
{ 
    char* temp = calloc( size, sizeof(uint8_t));
    memcpy( temp, buf + src_idx, size );
    memcpy( buf + dst_idx, temp, size );
    free( temp );
}