#if !defined(__NET_SERIALIZER_TESTER_H__)
#define __NET_SERIALIZER_TESTER_H__

#include "network_serializer.h"

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
    assert( test_net_serializer_init() );

    // Test U8
    {

    }
}

static bool test_net_serializer_init()
{
    net_serializer_init( &g_net_serializer );
    return 
    (
        (g_net_serializer.endianess == machine_endianness_type()) &&
        g_net_serializer._srz->buf &&
        g_net_serializer._srz->size
    );
}

static bool test_net_serializer_append_u8()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_insert_u8()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_copy_u8()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_remove_u8()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_append_u16()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_insert_u16()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_copy_u16()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_remove_u16()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_append_u32()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_insert_u32()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_copy_u32()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_remove_u32()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_append_u64()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_insert_u64()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_copy_u64()
{
    bool ret = true;

    return ret;
}

static bool test_net_serializer_remove_u64()
{
    bool ret = true;

    return ret;
}


#endif // __NET_SERIALIZER_TESTER_H__
