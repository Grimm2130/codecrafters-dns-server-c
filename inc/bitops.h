#if !defined(__BIT_OPS_H__)
#define __BIT_OPS_H__

#define IS_SET(n, pos)  ( ((n) >> (pos)) & 1 )  
#define TOGGLE_BIT(n, pos)  ((n) ^= (1 << (pos)))
#define COMPLEMENT8(num)    ((n) ^= 0xFF)
#define COMPLEMENT16(num)   ((n) ^= 0xFFFF) 
#define COMPLEMENT32(num)   ((n) ^= 0xFFFFFFFF) 
#define COMPLEMENT64(num)   ((n) ^= 0xFFFFFFFFFFFFFFFF) 
#define UNSET_BIT32(n, pos) ((n) &= ~(1 << (pos)))
#define UNSET_BIT64(n, pos) ((n) &= ~(1 << (pos)))
#define UNSET_BIT16(n, pos) ((n) &= ~(1 << (pos)))
#define UNSET_BIT8(n, pos)  ((n) &= ~(1 << (pos)))
#define SET_BIT(n, pos)     ((n) |= (1U << (pos)))

#include <stdint.h>

uint8_t endian_flip8( uint8_t n );
uint16_t endian_flip16( uint16_t n );
uint32_t endian_flip32( uint32_t n );
uint64_t endian_flip64( uint64_t n );

typedef enum machine_endian_type
{
    MACHINE_ENDIAN_TYPE_LITTLE_ENDIAN,
    MACHINE_ENDIAN_TYPE_BIG_ENDIAN
} machine_endian_type_t;

machine_endian_type_t machine_endianness_type();
    
#endif // __BIT_OPS_H__
