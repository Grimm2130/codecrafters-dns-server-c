#include "cir_buffer.h"
#include "utils.h"
#include "common.h"

cir_buffer_t *cir_buffer_create(const bool allow_overwrite)
{
    cir_buffer_t *cbuf = calloc(1, sizeof(cir_buffer_t));

    cir_buffer_init(cbuf, allow_overwrite);

    return cbuf;
}

void cir_buffer_init(cir_buffer_t *cbuf, const bool allow_overwrite)
{
    cbuf->allow_overwrite = allow_overwrite;
    cbuf->front = cbuf->rear = cbuf->size = 0;

    for (int i = 0; i < CIR_BUFFER_CAPACITY; i++)
    {
        cbuf->buffer[i] = NULL;
    }
}

bool cir_buffer_is_empty(cir_buffer_t *cbuf)
{
    return cbuf->size == 0;
}

bool cir_buffer_is_full(cir_buffer_t *cbuf)
{
    return cbuf->size == CIR_BUFFER_CAPACITY;
}

/// @brief Read from the circular buffer
/// @param cbuf Circular buffer object pointer
/// @param buf Buffer to write to
/// @param amnt Amount to be written
/// @return Number of bytes read successfully
int cir_buffer_read(cir_buffer_t *cbuf, char *buf, const uint32_t amnt)
{
    bool can_read = false;

    if (cir_buffer_is_empty(cbuf) == false)
    {
        can_read = true;
    }

    if (can_read)
    {
        memcpy(buf, cbuf->buffer[cbuf->front], amnt);
        free(cbuf->buffer[cbuf->front]);
        cbuf->buffer[cbuf->front] = NULL;
        cbuf->front = (cbuf->front + 1) % CIR_BUFFER_CAPACITY;
        cbuf->size--;
    }

    return amnt;
}

/// @brief Circular Buffer Write: Write to a circular buffer
/// @param cbuf Circular buffer object pointer
/// @param buf Buffer to read from
/// @param amnt Amount to read from buffer
/// @return number of bytes written
int cir_buffer_write(cir_buffer_t *cbuf, const char *buf, const uint32_t amnt)
{
    bool can_write = true;

    if (cir_buffer_is_full(cbuf))
    {
        // release current data at read index
        if (cbuf->allow_overwrite)
        {
            can_write = true;
            free(cbuf->buffer[cbuf->front]);
            cbuf->front = (cbuf->front + 1) % CIR_BUFFER_CAPACITY;
            cbuf->size--;
        }
        else
        {
            can_write = false;
        }
    }

    if (can_write)
    {
        cbuf->buffer[cbuf->rear] = calloc(amnt, sizeof(char));
        memcpy(cbuf->buffer[cbuf->rear], buf, amnt);
        cbuf->rear = (cbuf->rear + 1) % CIR_BUFFER_CAPACITY;
        cbuf->size++;
    }

    return amnt;
}

void cir_buffer_deinit(cir_buffer_t *cbuf)
{
    for (int i = 0; i < CIR_BUFFER_CAPACITY; i++)
    {
        if (cbuf->buffer[i])
            free(cbuf->buffer[i]);
        cbuf->buffer[i] = NULL;
    }
}

/// @brief Call if cirbuffer returned vai create;
/// @param cbuf
void cir_buffer_destroy(cir_buffer_t *cbuf)
{
    cir_buffer_deinit(cbuf);
    free(cbuf);
}
