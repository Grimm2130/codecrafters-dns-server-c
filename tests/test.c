#include "cir_buffer_tester.h"
#include "utils_tester.h"
#include "task_tester.h"
#include "sync_cir_buffer_tester.h"
#include "serializer_test.h"
#include "net_serializer_tester.h"

int main(int argc, char const *argv[])
{
    run_tests_cir_buffer();
    run_tests_utils();
    run_tests_task();
    run_test_sync_cir_buffer();
    run_test_serializer();
    run_tests_net_serializer();
    printf("All tests passed\n");
    return 0;
}
