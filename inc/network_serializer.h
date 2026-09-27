#if !defined(__NET_SERIALIZER_H__)
#define __NET_SERIALIZER_H__

#include "bitops.h"
#include <stdint.h>

typedef struct serializer serializer_t;

typedef struct net_serializer
{
    machine_endian_type_t endianess;    
    serializer_t * _srz;
} net_serializer_t;


net_serializer_t* net_serializer_create( );
void net_serializer_init( net_serializer_t* srz );

int net_serializer_append_u8( net_serializer_t* srz, const uint8_t *data, const uint32_t len );
int net_serializer_append_u16( net_serializer_t* srz, const uint16_t *data, const uint32_t len );
int net_serializer_append_u32( net_serializer_t* srz, const uint32_t *data, const uint32_t len );
int net_serializer_append_u64( net_serializer_t* srz, const uint64_t *data, const uint32_t len );
int net_serializer_append_raw( net_serializer_t* srz, const char *data, const uint32_t len );

int net_serializer_insert_u8( net_serializer_t* srz, const uint32_t start_idx, const uint8_t *data, const uint32_t len );
int net_serializer_insert_u16( net_serializer_t* srz, const uint32_t start_idx, const uint16_t *data, const uint32_t len );
int net_serializer_insert_u32( net_serializer_t* srz, const uint32_t start_idx, const uint32_t *data, const uint32_t len );
int net_serializer_insert_u64( net_serializer_t* srz, const uint32_t start_idx, const uint64_t *data, const uint32_t len );
int net_serializer_insert_raw( net_serializer_t* srz, const uint32_t start_idx, const char *data, const uint32_t len );

int net_serializer_remove_u8( net_serializer_t* srz, const uint32_t start_idx, uint8_t *data, const uint32_t len );
int net_serializer_remove_u16( net_serializer_t* srz, const uint32_t start_idx, uint16_t *data, const uint32_t len );
int net_serializer_remove_u32( net_serializer_t* srz, const uint32_t start_idx, uint32_t *data, const uint32_t len );
int net_serializer_remove_u64( net_serializer_t* srz, const uint32_t start_idx, uint64_t *data, const uint32_t len );
int net_serializer_remove_raw( net_serializer_t* srz, const uint32_t start_idx, char *data, const uint32_t len );

int net_serializer_copy_u8( net_serializer_t* srz, const uint32_t start_idx, uint8_t *data, const uint32_t len );
int net_serializer_copy_u16( net_serializer_t* srz, const uint32_t start_idx, uint16_t *data, const uint32_t len );
int net_serializer_copy_u32( net_serializer_t* srz, const uint32_t start_idx, uint32_t *data, const uint32_t len );
int net_serializer_copy_u64( net_serializer_t* srz, const uint32_t start_idx, uint64_t *data, const uint32_t len );
int net_serializer_copy_raw( net_serializer_t* srz, const uint32_t start_idx, char *data, const uint32_t len );

const char * net_serializer_getbuf( net_serializer_t * srz );

void net_serializer_clear( net_serializer_t* srz );
void net_serializer_deinit( net_serializer_t* srz );
void net_serializer_release( net_serializer_t* srz );

// int net_serializer_append_i8( net_serializer_t* srz, const int8_t *data, const uint32_t len );
// int net_serializer_append_i16( net_serializer_t* srz, const int16_t *data, const uint32_t len );
// int net_serializer_append_i32( net_serializer_t* srz, const int32_t *data, const uint32_t len );
// int net_serializer_append_i64( net_serializer_t* srz, const int64_t *data, const uint32_t len );
// int net_serializer_insert_i8( net_serializer_t* srz, const uint32_t start_idx, const int8_t *data, const uint32_t len );
// int net_serializer_insert_i16( net_serializer_t* srz, const uint32_t start_idx, const int16_t *data, const uint32_t len );
// int net_serializer_insert_i32( net_serializer_t* srz, const uint32_t start_idx, const int32_t *data, const uint32_t len );
// int net_serializer_insert_i64( net_serializer_t* srz, const uint32_t start_idx, const int64_t *data, const uint32_t len );
// int net_serializer_remove_i8( net_serializer_t* srz, const uint32_t start_idx, int8_t *data, const uint32_t len );
// int net_serializer_remove_i16( net_serializer_t* srz, const uint32_t start_idx, int16_t *data, const uint32_t len );
// int net_serializer_remove_i32( net_serializer_t* srz, const uint32_t start_idx, int32_t *data, const uint32_t len );
// int net_serializer_remove_i64( net_serializer_t* srz, const uint32_t start_idx, int64_t *data, const uint32_t len );
// int net_serializer_copy_i8( net_serializer_t* srz, const uint32_t start_idx, int8_t *data, const uint32_t len );
// int net_serializer_copy_i16( net_serializer_t* srz, const uint32_t start_idx, int16_t *data, const uint32_t len );
// int net_serializer_copy_i32( net_serializer_t* srz, const uint32_t start_idx, int32_t *data, const uint32_t len );
// int net_serializer_copy_i64( net_serializer_t* srz, const uint32_t start_idx, int64_t *data, const uint32_t len );
// int net_serializer_append_arr_u8( net_serializer_t* srz, const uint8_t *data, const uint32_t len );
// int net_serializer_append_arr_u16( net_serializer_t* srz, const uint16_t *data, const uint32_t len );
// int net_serializer_append_arr_u32( net_serializer_t* srz, const uint32_t *data, const uint32_t len );
// int net_serializer_append_arr_u64( net_serializer_t* srz, const uint64_t *data, const uint32_t len );
// int net_serializer_append_arr_i8( net_serializer_t* srz, const int8_t *data, const uint32_t len );
// int net_serializer_append_arr_i16( net_serializer_t* srz, const int16_t *data, const uint32_t len );
// int net_serializer_append_arr_i32( net_serializer_t* srz, const int32_t *data, const uint32_t len );
// int net_serializer_append_arr_i64( net_serializer_t* srz, const int64_t *data, const uint32_t len );
// int net_serializer_insert_u8( net_serializer_t* srz, const uint32_t start_idx, const uint8_t data );
// int net_serializer_insert_u16( net_serializer_t* srz, const uint32_t start_idx, const uint16_t data );
// int net_serializer_insert_u32( net_serializer_t* srz, const uint32_t start_idx, const uint32_t data );
// int net_serializer_insert_u64( net_serializer_t* srz, const uint32_t start_idx, const uint64_t data );
// int net_serializer_insert_i8( net_serializer_t* srz, const uint32_t start_idx, const int8_t data );
// int net_serializer_insert_i16( net_serializer_t* srz, const uint32_t start_idx, const int16_t data );
// int net_serializer_insert_i32( net_serializer_t* srz, const uint32_t start_idx, const int32_t data );
// int net_serializer_insert_i64( net_serializer_t* srz, const uint32_t start_idx, const int64_t data );
// int net_serializer_remove_u8( net_serializer_t* srz, const uint32_t start_idx, uint8_t *data );
// int net_serializer_remove_u16( net_serializer_t* srz, const uint32_t start_idx, uint16_t *data );
// int net_serializer_remove_u32( net_serializer_t* srz, const uint32_t start_idx, uint32_t *data );
// int net_serializer_remove_u64( net_serializer_t* srz, const uint32_t start_idx, uint64_t *data );
// int net_serializer_remove_i8( net_serializer_t* srz, const uint32_t start_idx, int8_t *data );
// int net_serializer_remove_i16( net_serializer_t* srz, const uint32_t start_idx, int16_t *data );
// int net_serializer_remove_i32( net_serializer_t* srz, const uint32_t start_idx, int32_t *data );
// int net_serializer_remove_i64( net_serializer_t* srz, const uint32_t start_idx, int64_t *data );
// int net_serializer_copy_arr_u8( net_serializer_t* srz );
// int net_serializer_copy_arr_u16( net_serializer_t* srz );
// int net_serializer_copy_arr_u32( net_serializer_t* srz );
// int net_serializer_copy_arr_u64( net_serializer_t* srz );
// int net_serializer_copy_arr_i8( net_serializer_t* srz );
// int net_serializer_copy_arr_i16( net_serializer_t* srz );
// int net_serializer_copy_arr_i32( net_serializer_t* srz );
// int net_serializer_copy_arr_i64( net_serializer_t* srz );

#endif // __NET_SERIALIZER_H__
