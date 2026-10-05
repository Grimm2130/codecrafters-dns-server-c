#if !defined(__NET_SERIALIZER_TESTER_H__)
#define __NET_SERIALIZER_TESTER_H__

#include "network_serializer.h"

#define NET_SER_ITEM_COUNT  255U

void run_tests_net_serializer();

static net_serializer_t g_net_serializer;

static bool test_net_serializer_init();
static bool test_net_serializer_append_u8();
static bool test_net_serializer_insert_u8();
static bool test_net_serializer_copy_u8();
static bool test_net_serializer_remove_u8();
static bool test_net_serializer_append_u16();
static bool test_net_serializer_insert_u16();
static bool test_net_serializer_copy_u16();
static bool test_net_serializer_remove_u16();
static bool test_net_serializer_append_u32();
static bool test_net_serializer_insert_u32();
static bool test_net_serializer_copy_u32();
static bool test_net_serializer_remove_u32();
static bool test_net_serializer_append_u64();
static bool test_net_serializer_insert_u64();
static bool test_net_serializer_copy_u64();
static bool test_net_serializer_remove_u64();

void run_tests_net_serializer()
{
    // Test intialize
    assert(test_net_serializer_init());
    
    // 8 bits
    assert(test_net_serializer_append_u8());
    assert(test_net_serializer_remove_u8());
    assert(test_net_serializer_insert_u8());
    assert(test_net_serializer_copy_u8());
    assert(test_net_serializer_remove_u8());
    
    assert(test_net_serializer_append_u16());
    assert(test_net_serializer_remove_u16());
    assert(test_net_serializer_insert_u16());
    assert(test_net_serializer_copy_u16());
    assert(test_net_serializer_remove_u16());

    assert(test_net_serializer_append_u32());
    assert(test_net_serializer_remove_u32());
    assert(test_net_serializer_insert_u32());
    assert(test_net_serializer_copy_u32());
    assert(test_net_serializer_remove_u32());

    assert(test_net_serializer_append_u64());
    assert(test_net_serializer_remove_u64());
    assert(test_net_serializer_insert_u64());
    assert(test_net_serializer_copy_u64());
    assert(test_net_serializer_remove_u64());
}

static bool test_net_serializer_init()
{
    net_serializer_init( &g_net_serializer );
    return 
    (
        (g_net_serializer.endianess == machine_endianness_type()) &&
        g_net_serializer._srz->buf &&
        (g_net_serializer._srz->size == 0)
    );
}

static bool test_net_serializer_append_u8()
{
    bool ret = true;

    for( uint8_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_append_u8( &g_net_serializer, &i, 1 );
    }

    assert( g_net_serializer._srz->size == NET_SER_ITEM_COUNT );

    return ret;
}

static bool test_net_serializer_insert_u8()
{
    bool ret = true;

    for( uint8_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_insert_u8( &g_net_serializer, 0, &i, 1 );
    }

    assert( g_net_serializer._srz->size == NET_SER_ITEM_COUNT );

    return ret;
}

static bool test_net_serializer_copy_u8()
{
    bool ret = true;
    uint8_t num;

    for( uint8_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_copy_u8( &g_net_serializer, sizeof(uint8_t) * (NET_SER_ITEM_COUNT-(i + 1)), &num, 1 );
        printf("Comparing 0x%.2x to 0x%.2x\n", i, num );
        assert( num == i );
    }

    return ret;
}

static bool test_net_serializer_remove_u8()
{
    bool ret = true;
    uint8_t num;

    for( uint8_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_remove_u8( &g_net_serializer, 0, &num, 1 );
    }

    assert( g_net_serializer._srz->size == 0 );

    return ret;
}

static bool test_net_serializer_append_u16()
{
    bool ret = true;

    for( uint16_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_append_u16( &g_net_serializer, &i, 1 );
    }

    assert( g_net_serializer._srz->size == (NET_SER_ITEM_COUNT * sizeof(uint16_t)) );

    return ret;
}

static bool test_net_serializer_insert_u16()
{
    bool ret = true;

    for( uint16_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_insert_u16( &g_net_serializer, 0, &i, 1 );
    }

    assert( g_net_serializer._srz->size == (NET_SER_ITEM_COUNT * sizeof(uint16_t)) );

    return ret;
}

static bool test_net_serializer_copy_u16()
{
    bool ret = true;
    uint16_t num;

    for( uint16_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_copy_u16( &g_net_serializer, sizeof(uint16_t) * (NET_SER_ITEM_COUNT-(i + 1)), &num, 1 );
        printf("Comparing 0x%.4x to 0x%.4x\n", i, num );
        assert( num == i );
    }

    return ret;
}

static bool test_net_serializer_remove_u16()
{
    bool ret = true;
    uint16_t num;

    for( uint16_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_remove_u16( &g_net_serializer, 0, &num, 1 );
    }

    assert( g_net_serializer._srz->size == 0 );

    return ret;
}

static bool test_net_serializer_append_u32()
{
    bool ret = true;

    for( uint32_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_append_u32( &g_net_serializer, &i, 1 );
    }

    assert( g_net_serializer._srz->size == (NET_SER_ITEM_COUNT * sizeof(uint32_t)));
    
    return ret;
}

static bool test_net_serializer_insert_u32()
{
    bool ret = true;

    for( uint32_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_insert_u32( &g_net_serializer, 0, &i, 1 );
    }

    assert( g_net_serializer._srz->size == (NET_SER_ITEM_COUNT * sizeof(uint32_t)) );

    return ret;
}

static bool test_net_serializer_copy_u32()
{
    bool ret = true;
    uint32_t num;

    for( uint32_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_copy_u32( &g_net_serializer, sizeof(uint32_t) * (NET_SER_ITEM_COUNT-(i + 1)), &num, 1 );
        printf("Comparing 0x%.8x to 0x%.8x\n", i, num );
        assert( num == i );
    }

    return ret;
}

static bool test_net_serializer_remove_u32()
{
    bool ret = true;
    uint32_t num;

    for( uint32_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_remove_u32( &g_net_serializer, 0, &num, 1 );
    }

    assert( g_net_serializer._srz->size == 0 );
    return ret;
}

static bool test_net_serializer_append_u64()
{
    bool ret = true;
    for( uint64_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_append_u64( &g_net_serializer, &i, 1 );
    }

    assert( g_net_serializer._srz->size == (NET_SER_ITEM_COUNT * sizeof(uint64_t)) );
    return ret;
}

static bool test_net_serializer_insert_u64()
{
    bool ret = true;

    for( uint64_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_insert_u64( &g_net_serializer, 0, &i, 1 );
    }

    assert( g_net_serializer._srz->size == (NET_SER_ITEM_COUNT * sizeof(uint64_t)) );

    return ret;
}

static bool test_net_serializer_copy_u64()
{
    bool ret = true;
    uint64_t num;

    for( uint64_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_copy_u64( &g_net_serializer, sizeof(uint64_t) * (NET_SER_ITEM_COUNT-(i + 1)), &num, 1 );
        printf("Comparing 0x%.16llx to 0x%.16llx\n", i, num );
        assert( num == i );
    }

    return ret;
}

static bool test_net_serializer_remove_u64()
{
    bool ret = true;
    uint64_t num;

    for( uint64_t i = 0; i < NET_SER_ITEM_COUNT; i++ )
    {
        net_serializer_remove_u64( &g_net_serializer, 0, &num, 1 );
    }

    assert( g_net_serializer._srz->size == 0 );
    return ret;
}


#endif // __NET_SERIALIZER_TESTER_H__
