#include "cir_buffer_tester.h"
#include "utils_tester.h"
#include "task_tester.h"
#include "sync_cir_buffer_tester.h"

int main(int argc, char const *argv[])
{
    run_tests_cir_buffer();
    run_tests_utils();
    run_tests_task();
    run_test_sync_cir_buffer();
    printf("All tests passed\n");
    return 0;
}
