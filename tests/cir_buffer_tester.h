#if !defined(__CIR_BUFFER_TESTER_H__)
#define __CIR_BUFFER_TESTER_H__

#include "cir_buffer.h"
#include "students.h"

#include <stdio.h>
#include <assert.h>


static cir_buffer_t cbuf;

void run_tests_cir_buffer();
static void test_cir_buffer_create();
static void test_cir_buffer_empty( bool expected );
static void test_cir_buffer_full( bool expected );
static void test_cir_buffer_read();
static void test_cir_buffer_write();

static void test_cir_buffer_create()
{
    cir_buffer_create_s(&cbuf, true);
    assert(cbuf.allow_overwrite == true);
    assert(cbuf.front == 0);
    assert(cbuf.rear == 0);
    assert(cbuf.size == 0);

    for(int i = 0; i < CIR_BUFFER_ENTRY_CAPACITY; i++ )
    {
        assert(cbuf.buffer[i] == NULL);
    }

}

static void test_cir_buffer_empty( bool expected )
{
    assert( cir_buffer_is_empty( &cbuf) == expected );
}

static void test_cir_buffer_full( bool expected )
{
    assert( cir_buffer_is_full(&cbuf) == expected );
}

static void test_cir_buffer_read()
{
    int count = 0;
    student_t student;
    while( cir_buffer_is_empty( &cbuf ) == false )
    {
        count++;
        cir_buffer_read( &cbuf, (char*)&student, STUDENT_SIZE);
        printf("Read " STUDENT_STR(&student));
    }

    printf("Read %d entries\n", count );
}

static void test_cir_buffer_write()
{
    int count = 0;

    while( cir_buffer_is_full( &cbuf ) == false )
    {
        cir_buffer_write( &cbuf, (char*)(&students[count]), STUDENT_SIZE);
        printf("Wrote " STUDENT_STR((&students[count])));
        count++;
    }

    printf("Read %d entries\n", count );
}


void run_tests_cir_buffer()
{
    
}

#endif // __CIR_BUFFER_TESTER_H__
