#if !defined(__SERIALIZER_H__)
#define __SERIALIZER_H__

#include <stdint.h>

/**
 * @brief srz Class
 * 
 * This class has the function of serilizing a sequence of elements and making the result of the serialization available
 * as a final buffer
 * 
 * The contents of the buffer can be retrieved at any time via the 'buf' pointer.
 * 
 * The length is updated for each new entry
 * 
 */

typedef struct serializer
{
    uint32_t size;
    uint32_t cap;
    uint8_t * buf;
} serializer_t;

serializer_t * serializer_create( );
void serializer_init( serializer_t *srz );
int serializer_append( serializer_t *srz, const char* data, int len );
int serializer_insert( serializer_t *srz, const uint32_t start_idx, const char* data, int len );
int serializer_remove( serializer_t *srz, const uint32_t start_idx, char* data, int len );
int serializer_copy( serializer_t *srz, const uint32_t start_idx, char* data, int len );
const char* serializer_getbuf( serializer_t * srz );
const char* serializer_dump( serializer_t *srz );
void serializer_clear( serializer_t *srz );
void serializer_deinit( serializer_t *srz );
void serializer_release( serializer_t *srz );

#endif // __SERIALIZER_H__
