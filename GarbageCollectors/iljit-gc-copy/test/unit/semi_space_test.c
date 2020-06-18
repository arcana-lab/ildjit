
/* For malloc/free */
#include <stdlib.h>

/* For test functions */
#include <semi_space_test.h>

/* We check semi_space component */
#include <semi_space.h>

/* Testing semispace default size */
#define SEMISPACE_SIZE 16 * sizeof(JITNUINT)

/* Memory where semispace live */
void* memory;

/* Used by non construction test, it is automatically initializiated */
semi_space_t space;

/* Alloc a new semispace */
void test_sm_setup(void) {

  /* Alloc and initialize semispace */
  memory = malloc(SEMISPACE_SIZE);
  sm_init(&space, memory, SEMISPACE_SIZE);

}

/* Free memory used during testing */
void test_sm_teardown(void) {

  free(memory);

}

/* Test sm_init with good values */
START_TEST(test_sm_init_ok) {

  /* Semispace must be equal to temp area */
  fail_unless(space.bottom == memory);
  fail_unless(space.size == SEMISPACE_SIZE);
  fail_unless(space.free_zone == space.bottom);

} END_TEST

/* Test sm_init with bad values */
START_TEST(test_sm_init_bad) {

  /* No way for check assertion "good" fails */

} END_TEST

/* Test sm_alloc_memory with good values */
START_TEST(test_sm_alloc_memory_ok) {

  /* New allocated memory */
  void* new_memory;

  /* Alloc memory */
  new_memory = sm_alloc_memory(&space, SEMISPACE_SIZE / 2);
  /* Test allocated memory size*/
  fail_unless(sm_used_memory(&space) == SEMISPACE_SIZE / 2);
  /* Try to fill all semispace */
  sm_alloc_memory(&space, SEMISPACE_SIZE / 2);
  fail_unless(sm_used_memory(&space) == SEMISPACE_SIZE);

} END_TEST

/* Test sm_alloc_memory with bad values */
START_TEST(test_sm_alloc_memory_bad) {

  /* New memory segment */
  void* new_memory;

  /* Try alloc 0 bytes */
  new_memory = sm_alloc_memory(&space, 0);
  fail_unless(new_memory == NULL);

} END_TEST

/* Test sm_add with good values */
START_TEST(test_sm_add_ok) {

  /* Test data */
  void* bytes;

  /* Alloc test data */
  bytes = malloc(SEMISPACE_SIZE / 4);
  sm_add(&space, bytes, SEMISPACE_SIZE / 4);
  /* Verify that something has been written */
  fail_unless(space.free_zone == space.bottom + SEMISPACE_SIZE / 4);
  /* Write a test to check written data? */
  /* Release test memory */
  free(bytes);

} END_TEST

/* Test sm_add with bad values */
START_TEST(test_sm_add_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test sm_reset with good values */
START_TEST(test_sm_reset_ok) {

  sm_reset(&space, sm_top(&space));
  fail_unless(space.free_zone == sm_top(&space));

} END_TEST

/* Test sm_reset with bad values */
START_TEST(test_sm_reset_bad) {

  /* No way for check assertion "good" fails */

} END_TEST

/* Test sm_top with good values */
START_TEST(test_sm_top_ok) {

  /* Compare sm_top output with the right top */
  fail_unless(space.bottom + space.size - 1 == sm_top(&space));

} END_TEST

/* Test sm_top with bad values */
START_TEST(test_sm_top_bad) {

  /* No way for check assertion "good" fails */

} END_TEST

/* Test sm_used_memory with good values */
START_TEST(test_sm_used_memory_ok) {

  /* For cycle counter */
  int i;

  /* Alloc some memory */
  for(i = 0; i < 2; i++)
    sm_alloc_memory(&space, SEMISPACE_SIZE / 4);
  /* Verify allocation */
  fail_unless(sm_used_memory(&space) == SEMISPACE_SIZE / 2);

} END_TEST

/* Test sm_used_memory with bad values */
START_TEST(test_sm_used_memory_bad) {

  /* No way for check assertion "good" fails */

} END_TEST

/* Test sm_free_memory with good values */
START_TEST(test_sm_free_memory_ok) {

  /* Alloc some memory */
  sm_alloc_memory(&space, SEMISPACE_SIZE / 2);
  /* Verify that allocation has been done */
  fail_unless(sm_free_memory(&space) == SEMISPACE_SIZE / 2);

} END_TEST

/* Test sm_free_memory with bad values */
START_TEST(test_sm_free_memory_bad) {

  /* No way for check assertion "good" fails */

} END_TEST

/* Test sm_valid_address with good values */
START_TEST(test_sm_valid_address_ok) {

  /* Check bounds */
  fail_unless(sm_valid_address(&space, space.bottom));
  fail_unless(sm_valid_address(&space, sm_top(&space)));
  /* Try to check an internal address */
  fail_unless(sm_valid_address(&space, space.bottom + SEMISPACE_SIZE / 2));

} END_TEST

/* Test sm_valid_address with bad values */
START_TEST(test_sm_valid_address_bad) {

  /* No way to check assertion "good" fails */

} END_TEST

/* Build a suite with all semi_space related test */
Suite* semi_space_suite(void) {

  Suite* suite;
  TCase* test_case;

  /* Build suite */
  suite = suite_create("semi_space");

  /* Build sm_init test cases */
  test_case = tcase_create("sm_init");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_init_ok);
  tcase_add_test(test_case, test_sm_init_bad);

  /* Build sm_alloc_memory test cases */
  test_case = tcase_create("sm_alloc_memory");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_alloc_memory_ok);
  tcase_add_test(test_case, test_sm_alloc_memory_bad);

  /* Build sm_add test cases */
  test_case = tcase_create("sm_add");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_add_ok);
  tcase_add_test(test_case, test_sm_add_bad);

  /* Build sm_reset test cases */
  test_case = tcase_create("sm_reset");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_reset_ok);
  tcase_add_test(test_case, test_sm_reset_bad);

  /* Build sm_top test cases */
  test_case = tcase_create("sm_top");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_top_ok);
  tcase_add_test(test_case, test_sm_top_bad);

  /* Build sm_used_memory test cases */
  test_case = tcase_create("sm_used_memory");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_used_memory_ok);
  tcase_add_test(test_case, test_sm_used_memory_bad);

  /* Build sm_free_memory test cases */
  test_case = tcase_create("sm_free_memory");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_free_memory_ok);
  tcase_add_test(test_case, test_sm_free_memory_bad);

  /* Build sm_valid_address test cases */
  test_case = tcase_create("sm_valid_address");
  tcase_add_checked_fixture(test_case, test_sm_setup, test_sm_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_sm_valid_address_ok);
  tcase_add_test(test_case, test_sm_valid_address_bad);

  return suite;

}
