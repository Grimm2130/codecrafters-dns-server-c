#include "bitmap.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

void bitmap_init( bitmap_t* bmap, const uint32_t size )
{
    assert( (size % 32) == 0 );
    bmap->bits = calloc( size/32, sizeof(uint32_t));
    bmap->size = size;
}

void bitmap_unset_bit_at( bitmap_t *bmap, uint32_t idx )
{
    int blk = idx / 32;
    int blk_idx = idx % 32;

    if( idx < bmap->size )
    {
        UNSET_BIT32(bmap->bits[blk], blk_idx);
    }
}

void bitmap_set_bit_at( bitmap_t *bmap, uint32_t idx )
{
    int blk = idx / 32;
    int blk_idx = idx % 32;

    if( idx < bmap->size )
    {
        // SET_BIT(bmap->bits[blk], blk_idx);
        bmap->bits[blk] |= 1 << blk_idx;
    }
}

bool bitmap_is_set_at( bitmap_t *bmap, uint32_t idx )
{
    int blk = idx / 32;
    int blk_idx = idx % 32;

    if( idx < bmap->size )
    {
        return IS_SET(bmap->bits[blk], blk_idx);
    }

    return false;
}

/// @brief Release the bitmap's internal array
/// @param bmap 
void bitmap_free_internal(bitmap_t *bmap)
{
    if( bmap->bits ) free( bmap->bits );
}

/// @brief Free the bitmap object
/// @param bmap 
void bitmap_free(bitmap_t *bmap)
{
    bitmap_free_internal( bmap );
    free( bmap );
}

/// @brief Reset all the blocks of the bitmap
/// @param bmap 
void bitmap_reset(bitmap_t *bmap) 
{
    for( int i = 0; i < bmap->size; i++ )
    {
        bmap->bits = 0;
    }
}

char *bitmap_print(bitmap_t *bmap)
{
    #define SBUFF_SIZE 4096U
    static char sbuf[ SBUFF_SIZE ];

    // clear the buffer
    memset( sbuf, 0, sizeof(char) * SBUFF_SIZE );

    assert( SBUFF_SIZE > bmap->size );
    int rc = 0;
    int c_idx;
    bool set;

    BITMAP_ITERATOR_BEGIN(bmap, 0, c_idx, set)

        if( c_idx && ((c_idx % 8) == 0) )
        {
            rc += snprintf( sbuf + rc, SBUFF_SIZE - rc, " " );
        }
        
        rc += snprintf( 
            sbuf + rc, 
            SBUFF_SIZE - rc, 
            set ? "1" : "0" 
        );
    BITMAP_ITERATOR_END

    return sbuf;
}