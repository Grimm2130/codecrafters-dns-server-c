#if !defined(__BIT_MAP_H__)
#define __BIT_MAP_H__

#include "bitops.h"
#include <stdint.h>
#include <stdbool.h>

/// @brief Bit map of 32 bit increments
typedef struct bitmap
{
    uint32_t size;
    uint32_t *bits;
} bitmap_t;

void bitmap_init( bitmap_t* bmap, const uint32_t size );
void bitmap_unset_bit_at( bitmap_t *bmap, uint32_t idx );
void bitmap_set_bit_at( bitmap_t *bmap, uint32_t idx );
bool bitmap_is_set_at( bitmap_t *bmap, uint32_t idx );
void bitmap_free_internal(bitmap_t *bmap);
void bitmap_free(bitmap_t *bmap);
void bitmap_reset(bitmap_t *bmap);
char * bitmap_print(bitmap_t *bmap);

#define BITMAP_ITERATOR_BEGIN( bitmap_ptr, start_idx, o_idx, o_bit )    \
{                                                                       \
    uint32_t i = start_idx;                                             \
    while( i < bitmap_ptr->size )                                       \
    {                                                                   \
        o_idx = i;                                                      \
        o_bit = bitmap_is_set_at( bitmap_ptr, i );                      \
        i++;                                                            \
    }                                                                   

#define BITMAP_ITERATOR_END }

#endif // __BIT_MAP_H__
