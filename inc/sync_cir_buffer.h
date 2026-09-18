#if !defined(__SYNC_CIR_BUFFER_H__)
#define __SYNC_CIR_BUFFER_H__

#include <pthread.h>
#include <stdint.h>

typedef struct cir_buffer cir_buffer_t;

typedef struct sync_cir_buffer
{
    pthread_mutex_t lock;
    pthread_cond_t read_avail;
    pthread_cond_t write_avail;
    cir_buffer_t* _cbuf;
} sync_cir_buffer_t;

sync_cir_buffer_t* sync_cir_buffer_create( bool allow_overwrite );
int sync_cir_buffer_init( sync_cir_buffer_t*, bool allow_overwrite );
bool sync_cir_buffer_is_empty( sync_cir_buffer_t* );
bool sync_cir_buffer_is_full( sync_cir_buffer_t* );
int sync_cir_buffer_read( sync_cir_buffer_t*, char *, const uint32_t amnt, bool broadcast_waiting );
int sync_cir_buffer_write( sync_cir_buffer_t*, const char *, const uint32_t amnt,  bool broadcast_waiting );
void sync_cir_buffer_deinit( sync_cir_buffer_t* );
void sync_cir_buffer_destroy( sync_cir_buffer_t* );

#endif // __SYNC_CIR_BUFFER_H__
