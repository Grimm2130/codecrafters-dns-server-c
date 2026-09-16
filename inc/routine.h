#if !defined(__ROUTINE_H__)
#define __ROUTINE_H__

#include <stdint.h>

typedef void* routine_arg;
typedef void* (*routine_fn)(void* arg);

typedef struct routine
{
    uint32_t prio;
    uint32_t stack_size;
    routine_arg _arg;
    routine_fn _fn;
} routine_t;

#endif // __ROUTINE_H__
