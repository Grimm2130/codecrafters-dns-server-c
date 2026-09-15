#if !defined(__CIR_BUFER_H__)
#define __CIR_BUFER_H__

/**
 * Circular buffer implementation
*/

#include "common.h"

#define CIR_BUFFER_ENTRY_CAPACITY   255U
#define CIR_BUFFER_ENTRY_SIZE       1024U

typedef struct cir_buffer
{
    bool allow_overwrite;
    uint32_t front;
    uint32_t rear;
    uint32_t size;
    char *buffer[CIR_BUFFER_ENTRY_CAPACITY];
} cir_buffer_t;

cir_buffer_t* cir_buffer_create( const bool allow_overwrite );
void cir_buffer_create_s( cir_buffer_t* cbuf,const bool allow_overwrite );

bool cir_buffer_is_empty( cir_buffer_t* );
bool cir_buffer_is_full( cir_buffer_t* );
int cir_buffer_read( cir_buffer_t*, char *, const uint32_t amnt );
int cir_buffer_write( cir_buffer_t*, const char *, const uint32_t amnt );
void cir_buffer_destroy( cir_buffer_t* );


#endif // __CIR_BUFER_H__
