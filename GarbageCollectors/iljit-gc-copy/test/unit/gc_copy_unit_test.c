
/* For EXIT_* */
#include <stdlib.h>

/* Unit test library */
#include <check.h>

/* Test suite definitions */
#include <semi_space_test.h>
#include <gc_memory_test.h>
#include <collector_test.h>
#include <iljit_gc_copy_test.h>
#include <recursion_stack_test.h>

/* Entry point */
int main(void) {

  /* Number of failed test */
  int fail_count;
 
  /* Can run test */
  SRunner *suite_runner;

  /* Add all test suite to suite runner */
  suite_runner = srunner_create(semi_space_suite());
  srunner_add_suite(suite_runner, gc_memory_suite());
  srunner_add_suite(suite_runner, collector_suite());
  srunner_add_suite(suite_runner, iljit_gc_copy_suite());
  srunner_add_suite(suite_runner, recursion_stack_suite());

  /* Run test */
  srunner_run_all(suite_runner, CK_NORMAL);
  fail_count = srunner_ntests_failed(suite_runner);

  /* Cleanup */
  srunner_free(suite_runner);

  return (fail_count == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}
