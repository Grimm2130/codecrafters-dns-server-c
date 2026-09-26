#include "bitops.h"

uint8_t endian_flip8( uint8_t n )
{
    return n;
    // return n = ((n & 0xF0) >> 4) | ((n & 0x0F) << 4);
}

uint16_t endian_flip16( uint16_t n )
{
    n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
    // n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
    return n;
}

uint32_t endian_flip32( uint32_t n )
{
    n = ((n & 0xFFFF0000) >> 16) | ((n & 0x0000FFFF) << 16);
    n = ((n & 0xFF00FF00) >> 8) | ((n & 0x00FF00FF) << 8);
    // n = ((n & 0xF0F0F0F0) >> 4) | ((n & 0x0F0F0F0F) << 4);
    return n;
}

uint64_t endian_flip64( uint64_t n )
{
    n = ((n & 0xFFFFFFFF00000000) >> 32) | ((n & 0x00000000FFFFFFFF) << 32);
    n = ((n & 0xFFFF0000FFFF0000) >> 16) | ((n & 0x0000FFFF0000FFFF) << 16);
    n = ((n & 0xFF00FF00FF00FF00) >> 8) | ((n & 0x00FF00FF00FF00FF) << 8);
    // n = ((n & 0xF0F0F0F0F0F0F0F0) >> 4) | ((n & 0x0F0F0F0F0F0F0F0F) << 4);
    return n;
}

machine_endian_type_t machine_endianness_type()
{
    uint8_t n = 0x1;
    return *((char*)&n) ? MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN : MACHINE_ENDIAN_TYPE_BIG_ENDIAN;
}