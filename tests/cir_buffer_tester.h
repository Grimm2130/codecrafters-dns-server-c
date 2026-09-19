#if !defined(__CIR_BUFFER_TESTER_H__)
#define __CIR_BUFFER_TESTER_H__

#include "cir_buffer.h"
#include "students.h"

#include <stdio.h>
#include <assert.h>

static cir_buffer_t cbuf;

void run_tests_cir_buffer();
static void test_cir_buffer_create();
static void test_cir_buffer_empty(bool expected);
static void test_cir_buffer_full(bool expected);
static void test_cir_buffer_read(int cnt);
static void test_cir_buffer_write(int cnt);

static void test_cir_buffer_create()
{
    cir_buffer_init(&cbuf, true);
    assert(cbuf.allow_overwrite == true);
    assert(cbuf.front == 0);
    assert(cbuf.rear == 0);
    assert(cbuf.size == 0);

    for (int i = 0; i < CIR_BUFFER_CAPACITY; i++)
    {
        assert(cbuf.buffer[i] == NULL);
    }
}

static void test_cir_buffer_empty(bool expected)
{
    assert(cir_buffer_is_empty(&cbuf) == expected);
}

static void test_cir_buffer_full(bool expected)
{
    assert(cir_buffer_is_full(&cbuf) == expected);
}

static void test_cir_buffer_read(int cnt)
{
    student_t student;
    int count = 0;
    while (count < cnt)
    {
        cir_buffer_read(&cbuf, (char *)&student, STUDENT_SIZE);
        printf("Read " STUDENT_STR(&student));
        count++;
    }

    printf("Read %d entries\n", count);
}

static void test_cir_buffer_write(int cnt)
{
    int count = 0;
    while ((count < cnt) && (count < STUDENT_COUNT))
    {
        cir_buffer_write(&cbuf, (char *)(&students[count]), STUDENT_SIZE);
        printf("Wrote " STUDENT_STR((&students[count])));
        count++;
    }

    printf("Write %d entries\n", count);
}

void run_tests_cir_buffer()
{
    test_cir_buffer_create();
    test_cir_buffer_empty(true);
    test_cir_buffer_full(false);

    test_cir_buffer_write(CIR_BUFFER_CAPACITY / 2);
    test_cir_buffer_empty(false);
    test_cir_buffer_full(false);

    test_cir_buffer_write((CIR_BUFFER_CAPACITY / 2) + 1);
    test_cir_buffer_empty(false);
    test_cir_buffer_full(true);

    test_cir_buffer_read((CIR_BUFFER_CAPACITY / 2) + 1);
    test_cir_buffer_empty(false);
    test_cir_buffer_full(false);

    test_cir_buffer_read(CIR_BUFFER_CAPACITY / 2);
    test_cir_buffer_empty(true);
    test_cir_buffer_full(false);

    test_cir_buffer_write(CIR_BUFFER_CAPACITY * 3);
    test_cir_buffer_empty(false);
    test_cir_buffer_full(true);

    test_cir_buffer_read(CIR_BUFFER_CAPACITY);
    test_cir_buffer_empty(true);
    test_cir_buffer_full(false);

    cir_buffer_deinit(&cbuf);
}

#endif // __CIR_BUFFER_TESTER_H__
