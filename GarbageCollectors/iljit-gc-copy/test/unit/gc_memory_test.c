
/* For NULL */
#include <stdlib.h>

/* For test functions */
#include <gc_memory_test.h>

/* Tested component header file */
#include <gc_memory.h>

/* Garbage collector heap size */
#define HEAP_SIZE 32 * sizeof(JITNUINT)

/* Tested component */
gc_memory_t memory;

/* Setup test environment */
void test_gcm_setup(void) {

  /* Init memory */
  gcm_init(&memory, HEAP_SIZE);

}

/* Reset test environment */
void test_gcm_teardown(void) {

  /* Destroy memory */
  gcm_destroy(&memory);

}

/* Test gcm_init with good values */
START_TEST(test_gcm_init_ok) {

  /* Used to test initialization with an odd size */
  gc_memory_t odd_memory;

  /* Test the global memory */
  fail_unless(gcm_size(&memory) == HEAP_SIZE);

  /* Try to pass an odd size */
  gcm_init(&odd_memory, HEAP_SIZE - 1);
  fail_unless(gcm_size(&odd_memory) == HEAP_SIZE);

  /* Release memory allocated by odd_memory */
  gcm_destroy(&odd_memory);

} END_TEST

/* Test gcm_init with bad values */
START_TEST(test_gcm_init_bad) {

  /* No way to check assertions fail */

} END_TEST

/* Test gcm_destroy with good values */
START_TEST(test_gcm_destroy_ok) {

  /* How i can check that memory has been released? */

} END_TEST

/* Test gcm_destroy with bad values */
START_TEST(test_gcm_destroy_bad) {

  /* No way to check assertion fails */

} END_TEST

/* Test gcm_alloc with good values */
START_TEST(test_gcm_alloc_ok) {

  /* Try to fit all memory */
  gcm_alloc(&memory, HEAP_SIZE / 2);
  fail_unless(gcm_used(&memory) == HEAP_SIZE / 2);

} END_TEST

/* Test gcm_alloc with bad values */
START_TEST(test_gcm_alloc_bad) {

  /* Test runned on semi_space test unit */

} END_TEST

/* Test gcm_move with good values */
START_TEST(test_gcm_move_ok) {

  /* Each semispace has HEAP_SIZE / 2 size. Try to move semispace / 4 */
  gcm_move(&memory, memory.online_semi_space->bottom, HEAP_SIZE / 2 / 4);
  fail_unless(sm_used_memory(memory.offline_semi_space) == HEAP_SIZE / 2 / 4);

} END_TEST

/* Test gcm_move with bad values */
START_TEST(test_gcm_move_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test gcm_swap with good values */
START_TEST(test_gcm_swap_ok) {

  /* Store a reference to old online semispace */
  semi_space_t* old_space;

  /* Alloc some memory on online semispace */
  gcm_alloc(&memory, HEAP_SIZE / 2 / 4);
  old_space = memory.online_semi_space;
  /* Swap semispaces */
  gcm_swap_semispaces(&memory);
  /* Test swapping */
  fail_unless(memory.offline_semi_space == old_space);
  /* The new offline semispace must be all free */
  fail_unless(sm_used_memory(memory.offline_semi_space) == 0);

} END_TEST

/* Test gcm_swap with bad values */
START_TEST(test_gcm_swap_bad) {

  /* No way to check assertion fails */

} END_TEST

/* Test gcm_size with good values */
START_TEST(test_gcm_size_ok) {

  /* Verify that expected and real size are equals */
  fail_unless(gcm_size(&memory) == HEAP_SIZE);

} END_TEST

/* Test gcm_size with bad values */
START_TEST(test_gcm_size_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test gcm_used with god values */
START_TEST(test_gcm_used_ok) {

  /* An empty heap has 0 used bytes */
  fail_unless(gcm_used(&memory) == 0);
  /* Try to alloc some memory and see used memory count grow */
  gcm_alloc(&memory, HEAP_SIZE / 4 );
  fail_unless(gcm_used(&memory) == HEAP_SIZE / 4);

} END_TEST

/* Test gcm_used with bad values */
START_TEST(test_gcm_used_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Build a suite with all gc_memory_t related test */
Suite* gc_memory_suite(void) {

  Suite* suite;
  TCase* test_case;

  /* Build suite */
  suite = suite_create("gc_memory");

  /* Build gcm_init test cases */
  test_case = tcase_create("gcm_init");
  tcase_add_checked_fixture(test_case, test_gcm_setup, test_gcm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_gcm_init_ok);
  tcase_add_test(test_case, test_gcm_init_bad);

  /* Build gcm_destroy test cases */
  test_case = tcase_create("gcm_destroy");
  tcase_add_checked_fixture(test_case, test_gcm_setup, test_gcm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_gcm_destroy_ok);
  tcase_add_test(test_case, test_gcm_destroy_bad);

  /* Build gcm_alloc test cases */
  test_case = tcase_create("gcm_alloc");
  tcase_add_checked_fixture(test_case, test_gcm_setup, test_gcm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_gcm_alloc_ok);
  tcase_add_test(test_case, test_gcm_alloc_bad);

  /* Build gcm_move test cases */
  test_case = tcase_create("gmc_move");
  tcase_add_checked_fixture(test_case, test_gcm_setup, test_gcm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_gcm_move_ok);
  tcase_add_test(test_case, test_gcm_move_bad);

  /* Build gcm_swap_semispaces test cases */
  test_case = tcase_create("gcm_swap_semispaces");
  tcase_add_checked_fixture(test_case, test_gcm_setup, test_gcm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_gcm_swap_ok);
  tcase_add_test(test_case, test_gcm_swap_bad);

  /* Build gcm_size related tests */
  test_case = tcase_create("gcm_size");
  tcase_add_checked_fixture(test_case, test_gcm_setup, test_gcm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_gcm_size_ok);
  tcase_add_test(test_case, test_gcm_size_bad);

  /* Build gcm_used relates tests */
  test_case = tcase_create("gcm_used");
  tcase_add_checked_fixture(test_case, test_gcm_setup, test_gcm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_gcm_used_ok);
  tcase_add_test(test_case, test_gcm_used_bad);

  return suite;

}
