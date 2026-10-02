#if !defined(__SERIALIZER_TEST_H__)
#define __SERIALIZER_TEST_H__

#include "serializer.h"
#include "students.h"
#include <assert.h>
#include <stdbool.h>

static serializer_t g_serializer;

const char* student_str( const student_t* st )
{
    static char buffer[255];
    snprintf(buffer, 255, "{id: %d, first_name: %s, last_name: %s, age: %d}", 
        (st)->id, 
        (st)->first_name,
        (st)->last_name,
        (st)->age
    );

    return buffer;
}

void run_test_serializer();

static bool test_serializer_initialize();
static bool test_serializer_append();
static bool test_serializer_insert();
static bool test_serializer_copy();
static bool test_serializer_remove();
static bool test_serializer_deinit();

void run_test_serializer()
{
    printf("Serializer tests running...\n");
    assert(test_serializer_initialize());
    
    assert(test_serializer_append());
    assert(test_serializer_remove());

    assert(test_serializer_insert());
    assert(test_serializer_copy());
    assert(test_serializer_remove());
    
    assert(test_serializer_deinit());
    printf("Serializer tests complete\n");
}

static bool test_serializer_initialize()
{
    bool ret = true;

    serializer_init(&g_serializer);

    return ret;
}

static bool test_serializer_append()
{
    bool ret = true;
    student_t* c_student;

    for( int i = 0; i < STUDENT_COUNT; i++ )
    {
        c_student = &students[i];
        if(serializer_append(&g_serializer, (const char*)c_student, STUDENT_SIZE ))
        {
            ret = false;
            break;
        }
    }

    return ret;
}

static bool test_serializer_insert()
{
    bool ret = true;

    student_t* c_student;

    for( int i = 0; i < STUDENT_COUNT; i++ )
    {
        c_student = &students[i];
        if(serializer_insert(&g_serializer, 0, (const char*)c_student, STUDENT_SIZE ))
        {
            ret = false;
            break;
        }
    }

    return ret;
}

static bool test_serializer_copy()
{
    bool ret = true;

    student_t* c_student_ptr;
    student_t c_student;
    
    for( int i = 0; i < STUDENT_COUNT; i++ )
    {
        c_student_ptr = &students[i];

        serializer_copy(&g_serializer, STUDENT_SIZE * (STUDENT_COUNT - (i + 1)), (char*)&c_student, STUDENT_SIZE );

        printf("Comparing %s to %s\n", student_str(c_student_ptr), student_str(&c_student) );

        if(STUDENT_CMP( &c_student, c_student_ptr ) == true)
        {
            ret = false;
            break;
        }
    }

    return ret;
}

static bool test_serializer_remove()
{
    bool ret = true;

    student_t* c_student_ptr;
    student_t c_student;
    
    for( int i = 0; i < STUDENT_COUNT; i++ )
    {
        serializer_remove(&g_serializer, 0, (char*)&c_student, STUDENT_SIZE );
    }

    ret = g_serializer.size == 0;

    return ret;
}

static bool test_serializer_deinit()
{
    bool ret = true;
    
    serializer_deinit(&g_serializer);

    return ret;
}

#endif // __SERIALIZER_TEST_H__
