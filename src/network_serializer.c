#include "network_serializer.h"
#include "serializer.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define NET_SERIALIZER_AVAILABLE_SIZE(net_srz_ptr) ((net_srz_ptr)->_srz->cap - (net_srz_ptr)->_srz->size)

net_serializer_t *net_serializer_create()
{
    net_serializer_t *nsrz = calloc(1, sizeof(net_serializer_t));
    net_serializer_init(nsrz);
    return nsrz;
}

void net_serializer_init(net_serializer_t *srz)
{
    srz->_srz = serializer_create();
    srz->endianess = machine_endianness_type();
}

uint32_t net_serializer_get_size(net_serializer_t *srz)
{
    return serializer_get_size(srz->_srz);
}

int net_serializer_append_u8(net_serializer_t *srz, const uint8_t *data, const uint32_t len)
{
    uint8_t data_end_aware;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip8(data[i]) : data[i];
        serializer_append(srz->_srz, (const char *)&data_end_aware, sizeof(uint8_t));
    }

    return 0;
}

int net_serializer_append_u16(net_serializer_t *srz, const uint16_t *data, const uint32_t len)
{
    uint16_t data_end_aware;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip16(data[i]) : data[i];
        serializer_append(srz->_srz, (const char *)&data_end_aware, sizeof(uint16_t));
    }

    return 0;
}

int net_serializer_append_u32(net_serializer_t *srz, const uint32_t *data, const uint32_t len)
{
    uint32_t data_end_aware;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip32(data[i]) : data[i];
        serializer_append(srz->_srz, (const char *)&data_end_aware, sizeof(uint32_t));
    }

    return 0;
}

int net_serializer_append_u64(net_serializer_t *srz, const uint64_t *data, const uint32_t len)
{
    uint64_t data_end_aware;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip64(data[i]) : data[i];
        serializer_append(srz->_srz, (const char *)&data_end_aware, sizeof(uint64_t));
    }

    return 0;
}

int net_serializer_insert_u8(net_serializer_t *srz, const uint32_t start_idx, const uint8_t *data, const uint32_t len)
{
    uint8_t data_end_aware;
    uint32_t offset = 0;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip8(data[i]) : data[i];
        serializer_insert(srz->_srz, start_idx + offset, (const char *)&data_end_aware, sizeof(uint8_t));
        offset += sizeof(uint8_t);
    }

    return 0;
}

int net_serializer_insert_u16(net_serializer_t *srz, const uint32_t start_idx, const uint16_t *data, const uint32_t len)
{
    uint16_t data_end_aware;
    uint32_t offset = 0;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip16(data[i]) : data[i];
        serializer_insert(srz->_srz, start_idx + offset, (const char *)&data_end_aware, sizeof(uint16_t));
        offset += sizeof(uint16_t);
    }

    return 0;
}

int net_serializer_insert_u32(net_serializer_t *srz, const uint32_t start_idx, const uint32_t *data, const uint32_t len)
{
    uint32_t data_end_aware;
    uint32_t offset = 0;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip32(data[i]) : data[i];
        serializer_insert(srz->_srz, start_idx + offset, (const char *)&data_end_aware, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }

    return 0;
}

int net_serializer_insert_u64(net_serializer_t *srz, const uint32_t start_idx, const uint64_t *data, const uint32_t len)
{
    uint64_t data_end_aware;
    uint32_t offset = 0;

    if (len > NET_SERIALIZER_AVAILABLE_SIZE(srz))
        return 1;

    for (int i = 0; i < len; i++)
    {
        data_end_aware = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip64(data[i]) : data[i];
        serializer_insert(srz->_srz, start_idx + offset, (const char *)&data_end_aware, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }

    return 0;
}

int net_serializer_remove_u8(net_serializer_t *srz, const uint32_t start_idx, uint8_t *data, const uint32_t len)
{
    uint8_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_remove(srz->_srz, start_idx, (char *)&data_end_aware, sizeof(uint8_t));
        if (data)
        {
            data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip8(data_end_aware) : data_end_aware;
        }
        offset += sizeof(uint8_t);
    }

    return 0;
}

int net_serializer_remove_u16(net_serializer_t *srz, const uint32_t start_idx, uint16_t *data, const uint32_t len)
{
    uint16_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_remove(srz->_srz, start_idx, (char *)&data_end_aware, sizeof(uint16_t));
        if (data)
        {
            data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip16(data_end_aware) : data_end_aware;
        }
        offset += sizeof(uint16_t);
    }

    return 0;
}

int net_serializer_remove_u32(net_serializer_t *srz, const uint32_t start_idx, uint32_t *data, const uint32_t len)
{
    uint32_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_remove(srz->_srz, start_idx, (char *)&data_end_aware, sizeof(uint32_t));
        if (data)
        {
            data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip32(data_end_aware) : data_end_aware;
        }
        offset += sizeof(uint32_t);
    }

    return 0;
}

int net_serializer_remove_u64(net_serializer_t *srz, const uint32_t start_idx, uint64_t *data, const uint32_t len)
{
    uint64_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_remove(srz->_srz, start_idx, (char *)&data_end_aware, sizeof(uint64_t));
        if (data)
        {
            data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip64(data_end_aware) : data_end_aware;
        }
        offset += sizeof(uint64_t);
    }

    return 0;
}

int net_serializer_copy_u8(net_serializer_t *srz, const uint32_t start_idx, uint8_t *data, const uint32_t len)
{
    uint8_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_copy(srz->_srz, start_idx + offset, (char *)&data_end_aware, sizeof(uint8_t));
        data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip8(data_end_aware) : data_end_aware;
        offset += sizeof(uint8_t);
    }

    return 0;
}

int net_serializer_copy_u16(net_serializer_t *srz, const uint32_t start_idx, uint16_t *data, const uint32_t len)
{
    uint16_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_copy(srz->_srz, start_idx + offset, (char *)&data_end_aware, sizeof(uint16_t));
        data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip16(data_end_aware) : data_end_aware;
        offset += sizeof(uint16_t);
    }

    return 0;
}

int net_serializer_copy_u32(net_serializer_t *srz, const uint32_t start_idx, uint32_t *data, const uint32_t len)
{
    uint32_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_copy(srz->_srz, start_idx + offset, (char *)&data_end_aware, sizeof(uint32_t));
        data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip32(data_end_aware) : data_end_aware;
        offset += sizeof(uint32_t);
    }

    return 0;
}

int net_serializer_copy_u64(net_serializer_t *srz, const uint32_t start_idx, uint64_t *data, const uint32_t len)
{
    uint64_t data_end_aware;
    uint32_t offset = 0;

    if (len > srz->_srz->size)
        return 1;

    for (int i = 0; i < len; i++)
    {
        serializer_copy(srz->_srz, start_idx + offset, (char *)&data_end_aware, sizeof(uint64_t));
        data[i] = srz->endianess == MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN ? endian_flip64(data_end_aware) : data_end_aware;
        offset += sizeof(uint64_t);
    }

    return 0;
}

int net_serializer_append_raw(net_serializer_t *srz, const char *data, const uint32_t len)
{
    return serializer_append(srz->_srz, data, len);
}

int net_serializer_insert_raw(net_serializer_t *srz, const uint32_t start_idx, const char *data, const uint32_t len)
{
    return serializer_insert(srz->_srz, start_idx, data, len);
}

int net_serializer_remove_raw(net_serializer_t *srz, const uint32_t start_idx, char *data, const uint32_t len)
{
    return serializer_remove(srz->_srz, start_idx, data, len);
}

int net_serializer_copy_raw(net_serializer_t *srz, const uint32_t start_idx, char *data, const uint32_t len)
{
    return serializer_copy(srz->_srz, start_idx, data, len);
}

const char *net_serializer_get_buf(net_serializer_t *srz)
{
    return serializer_get_buf(srz->_srz);
}

const char *net_serializer_get_buf_from_offset(net_serializer_t *srz, const uint32_t offset)
{
    return serializer_get_buf_from_offset(srz->_srz, offset);
}

void net_serializer_clear(net_serializer_t *srz)
{
    serializer_clear(srz->_srz);
}

void net_serializer_deinit(net_serializer_t *srz)
{
    serializer_release(srz->_srz);
}

void net_serializer_release(net_serializer_t *srz)
{
    net_serializer_deinit(srz);
    free(srz);
}
