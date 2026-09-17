#include "cir_buffer.h"
#include "sync_cir_buffer.h"


#define LOCK(buf)       (pthread_mutex_lock( &((buf)->lock)) )
#define UNLOCK(buf)       (pthread_mutex_unlock( &((buf)->lock)) )

#define SIGNAL(buf)       (pthread_cond_signal( &((buf)->signal)) )
#define BROADCAST(buf)       (pthread_cond_broadcast( &((buf)->signal)) )
#define WAIT(buf)       (pthread_cond_wait( &((buf)->signal), &((buf)->lock) ) )

sync_cir_buffer_t* sync_cir_buffer_create( bool allow_overwrite )
{
    sync_cir_buffer_t * buf = calloc(1, sizeof(sync_cir_buffer_t));
    if( sync_cir_buffer_init(buf, allow_overwrite) )
    {
        free( buf );
        buf = NULL;
    }

    return NULL;
}

int sync_cir_buffer_init( sync_cir_buffer_t* buf, bool allow_overwrite )
{
    int ret;
    buf->_cbuf = cir_buffer_create(allow_overwrite);

    if( (ret = pthread_mutex_init( &buf->lock, NULL)) == 0 )
    {
        if( (ret = pthread_cond_init( &buf->signal, NULL )) == 0 )
        {
            //
        }
    }

    return ret;
}

bool sync_cir_buffer_is_empty( sync_cir_buffer_t* buf)
{
    LOCK(buf);
    bool res = cir_buffer_is_empty( buf->_cbuf );
    UNLOCK(buf);
    return res;
}

bool sync_cir_buffer_is_full( sync_cir_buffer_t* buf)
{
    LOCK(buf);
    bool res = cir_buffer_is_full( buf->_cbuf );
    UNLOCK(buf);
    return res;
}

int sync_cir_buffer_read( sync_cir_buffer_t* buf, char * data, const uint32_t amnt )
{
    LOCK(buf);
    int res = cir_buffer_read( buf->_cbuf, data, amnt );
    UNLOCK(buf);
    return res;
}

int sync_cir_buffer_write( sync_cir_buffer_t* buf, const char * data, const uint32_t amnt, bool broadcast )
{
    LOCK(buf);
    int res = cir_buffer_write( buf->_cbuf, data, amnt );
    UNLOCK(buf);

    if( broadcast ) BROADCAST(buf);
    else SIGNAL( buf );
    
    return res;
}

void sync_cir_buffer_await_data( sync_cir_buffer_t* buf )
{
    LOCK( buf );
    
    while( cir_buffer_is_empty( buf->_cbuf ) )
    {
        WAIT(buf);
    }


    UNLOCK( buf );
}

void sync_cir_buffer_deinit( sync_cir_buffer_t* buf )
{
    LOCK(buf);
    cir_buffer_destroy( buf->_cbuf );
    buf->_cbuf = NULL;
    UNLOCK(buf);
}

void sync_cir_buffer_destroy( sync_cir_buffer_t* buf )
{
    sync_cir_buffer_deinit( buf);
    free(buf);
}
