#include "cir_buffer.h"
#include "sync_cir_buffer.h"

#define LOCK(buf) (pthread_mutex_lock(&((buf)->lock)))
#define UNLOCK(buf) (pthread_mutex_unlock(&((buf)->lock)))

#define SIGNAL_READ(buf) (pthread_cond_signal(&((buf)->read_avail)))
#define BROADCAST_READ(buf) (pthread_cond_broadcast(&((buf)->read_avail)))
#define WAIT_READ(buf) (pthread_cond_wait(&((buf)->read_avail), &((buf)->lock)))

#define SIGNAL_WRITE(buf) (pthread_cond_signal(&((buf)->write_avail)))
#define BROADCAST_WRITE(buf) (pthread_cond_broadcast(&((buf)->write_avail)))
#define WAIT_WRITE(buf) (pthread_cond_wait(&((buf)->write_avail), &((buf)->lock)))

sync_cir_buffer_t *sync_cir_buffer_create(bool allow_overwrite)
{
    sync_cir_buffer_t *buf = calloc(1, sizeof(sync_cir_buffer_t));

    if (sync_cir_buffer_init(buf, allow_overwrite))
    {
        free(buf);
        buf = NULL;
    }

    return buf;
}

int sync_cir_buffer_init(sync_cir_buffer_t *buf, bool allow_overwrite)
{
    int ret;

    buf->_cbuf = cir_buffer_create(allow_overwrite);

    if ((ret = pthread_mutex_init(&buf->lock, NULL)) == 0)
    {
        if ((ret = pthread_cond_init(&buf->read_avail, NULL)) == 0)
        {
            if ((ret = pthread_cond_init(&buf->write_avail, NULL)) == 0)
            {
                //
            }
        }
    }

    return ret;
}

bool sync_cir_buffer_is_empty(sync_cir_buffer_t *buf)
{
    LOCK(buf);
    bool res = cir_buffer_is_empty(buf->_cbuf);
    UNLOCK(buf);
    return res;
}

bool sync_cir_buffer_is_full(sync_cir_buffer_t *buf)
{
    LOCK(buf);
    bool res = cir_buffer_is_full(buf->_cbuf);
    UNLOCK(buf);
    return res;
}

int sync_cir_buffer_read(sync_cir_buffer_t *buf, char *data, const uint32_t amnt, bool broadcast_waiting)
{
    LOCK(buf);

    // Wait for new data to be available
    while (cir_buffer_is_empty(buf->_cbuf))
    {
        WAIT_READ(buf);
    }

    int res = cir_buffer_read(buf->_cbuf, data, amnt);

    UNLOCK(buf);

    // signal that a write can be made (data was taken off the queue)
    if (broadcast_waiting)
        BROADCAST_WRITE(buf);
    else
        SIGNAL_WRITE(buf);

    return res;
}

int sync_cir_buffer_write(sync_cir_buffer_t *buf, const char *data, const uint32_t amnt, bool broadcast_waiting)
{
    LOCK(buf);

    if( buf->_cbuf->allow_overwrite == false )
    {
        // Wait for new data to be available
        while (cir_buffer_is_full(buf->_cbuf))
        {
            WAIT_WRITE(buf);
        }
    }

    int res = cir_buffer_write(buf->_cbuf, data, amnt);

    UNLOCK(buf);

    // signal that a read can be made (data was put on the queue)
    if (broadcast_waiting)
        BROADCAST_READ(buf);
    else
        SIGNAL_READ(buf);

    return res;
}

void sync_cir_buffer_clear(sync_cir_buffer_t * buf)
{
    LOCK(buf);
    cir_buffer_clear(buf->_cbuf);
    UNLOCK(buf);
}

void sync_cir_buffer_deinit(sync_cir_buffer_t *buf)
{
    LOCK(buf);
    cir_buffer_destroy(buf->_cbuf);
    buf->_cbuf = NULL;
    UNLOCK(buf);

    pthread_mutex_destroy(&buf->lock);
    pthread_cond_destroy(&buf->read_avail);
    pthread_cond_destroy(&buf->write_avail);
}

void sync_cir_buffer_destroy(sync_cir_buffer_t *buf)
{
    sync_cir_buffer_deinit(buf);
    free(buf);
}
