
/* For NULL */
#include <stdio.h>

/* For test functions */
#include <recursion_stack_test.h>

/* For memory allocation routines */
#include <gc_copy_utils.h>

/* Component to check */
#include <recursion_stack.h>

/* References set size */
#define REFERENCES_COUNT 2

/* References set indexes */
#define R1 0
#define R2 1

/* Pointer to tested object */
recursion_stack_t* stack;

/* Raw references set */
void* references[REFERENCES_COUNT];

/* A list of pointers */
XanList* pointers;

/* Initialize test environment */
void test_rs_setup(void) {

  /* Build an empty stack */
  stack = rs_init(NULL);

  /* Build references set */
  references[R1] = NULL;
  references[R2] = NULL;
  
  /* Fill in pointers list */
  pointers = xanListNew(xan_alloc, xfree, NULL);
  pointers->insert(pointers, NULL);
  pointers->insert(pointers, &references[R1]);
  pointers->insert(pointers, NULL);
  pointers->insert(pointers, &references[R2]);

}

/* Reset test environment */
void test_rs_teardown(void) {

  /* Delete stack */
  rs_destroy(stack);

  /* Delete pointers list */
  pointers->destroyList(pointers);

}

/* Test rs_init with right values */
START_TEST(test_rs_init_ok) {

  /* By defaul stack is empty */
  fail_unless(rs_is_empty(stack));

  /* Try to build a non empty stack */
  rs_destroy(stack);
  stack = rs_init(pointers);
  fail_unless(rs_size(stack) == REFERENCES_COUNT);

} END_TEST

/* Test rs_init with wrong values */
START_TEST(test_rs_init_bad) {

  /* Creation never fails */

} END_TEST

/* Test rs_destroy with right values */
START_TEST(test_rs_destroy_ok) {

  /* I don't know how to check if memory has been released */

} END_TEST

/* Test rs_destroy with wrong values */
START_TEST(test_rs_destroy_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test rs_push with right values */
START_TEST(test_rs_push_ok) {

  /* Stack size before growing */
  JITNUINT old_size;

  /* Pointer container */
  XanListItem* item;

  /* Save size */
  old_size = rs_size(stack);

  /* After a push stack size must increase */
  item = pointers->first(pointers);
  rs_push(stack, item->data);
  fail_unless(rs_size(stack) == old_size + 1);

} END_TEST

/* Test rs_push with bad values */
START_TEST(test_rs_push_bad) {

  /* Size before pushing */
  JITNUINT old_size;

  /* Routine return value */
  JITNUINT retval;

  /* Save old size */
  old_size = rs_size(stack);

  /* Pushing NULL values don't modify stack state */
  retval = rs_push(stack, NULL);
  fail_unless(retval == RS_PUSH_NULL);
  fail_unless(rs_size(stack) == old_size);

} END_TEST

/* Test rs_push_list with right values */
START_TEST(test_rs_push_list_ok) {

  /* Try adding a set of pointers */
  rs_push_list(stack, pointers);
  fail_unless(rs_size(stack) == REFERENCES_COUNT);

} END_TEST

/* Test rs_push_list with wrong values */
START_TEST(test_rs_push_list_bad) {

  /* Routine return value */
  JITNUINT retval;

  /* Stack size before growing */
  JITNUINT old_size;

  /* Save stack size */
  old_size = rs_size(stack);

  /* Try adding a NULL list */
  retval = rs_push(stack, NULL);
  fail_unless(retval == RS_PUSH_NULL);
  fail_unless(rs_size(stack) == old_size);
	
} END_TEST

/* Test rs_pop with right values */
START_TEST(test_rs_pop_ok) {

  /* Stack top */
  void* pointer;

  /* Stack size before pop operation */
  JITNUINT old_size;

  /* Push some data and save stack size */
  rs_push(stack, pointers);
  old_size = rs_size(stack);

  /* Pop: size must decrease and a non NULL pointer is returned */
  pointer = rs_pop(stack);
  fail_unless(pointer != NULL);
  fail_unless(rs_size(stack) == old_size - 1);

} END_TEST

/* Test rs_pop with wrong values */
START_TEST(test_rs_pop_bad) {

  /* Try poping from an empty stack */
  fail_unless(rs_pop(stack) == NULL);
  fail_unless(rs_pop(stack) == 0);

} END_TEST

/* Test rs_contains with right values */
START_TEST(test_rs_contains_ok) {

  /* Push a pointer and search it in the stack */
  rs_push(stack, &references[R1]);
  fail_unless(rs_contains(stack, &references[R1]));

  /* Try to find a never added pointer */
  fail_unless(!rs_contains(stack, &references[R2]));
	  
} END_TEST

/* Test rs_contains with wrong values */
START_TEST(test_rs_contains_bad) {

  /* Try to find NULL */
  fail_unless(!rs_contains(stack, NULL));

} END_TEST

/* Test rs_size with right values */
START_TEST(test_rs_size_ok) {

  /* Old size */
  JITNUINT old_size;

  /* Save stack size before grow */
  old_size = rs_size(stack);

  /* Grow and check that size increase */
  rs_push(stack, &references[R1]);
  fail_unless(rs_size(stack) == ++old_size);

  /* Reverse operation */
  rs_pop(stack);
  fail_unless(rs_size(stack) == --old_size);

} END_TEST

/* Test rs_size with wrong values */
START_TEST(test_rs_size_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test rs_is_empty with right values */
START_TEST(test_rs_is_empty_ok) {

  /* At start the stack is empty */
  fail_unless(rs_is_empty(stack));

  /* Add a value and redo the test */
  rs_push(stack, &references[R1]);
  fail_unless(!rs_is_empty(stack));

} END_TEST

/* Test rs_is_empty with wrong values */
START_TEST(test_rs_is_empty_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Build a suite with all recursion stack related tests */
Suite* recursion_stack_suite(void) {

  Suite* suite;
  TCase* test_case;

  /* Build suite */
  suite = suite_create("recursion_stack");

  /* Build rs_init test cases */
  test_case = tcase_create("rs_init");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_init_ok);
  tcase_add_test(test_case, test_rs_init_bad);

  /* Build rs_destroy test cases */
  test_case = tcase_create("rs_destroy");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_destroy_ok);
  tcase_add_test(test_case, test_rs_destroy_bad);
  
  /* Build rs_push test cases */
  test_case = tcase_create("rs_push");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_push_ok);
  tcase_add_test(test_case, test_rs_push_bad);

  /* Build rs_push_list test cases */
  test_case = tcase_create("rs_push_list");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_push_list_ok);
  tcase_add_test(test_case, test_rs_push_list_bad);
  
  /* Build rs_pop test cases */
  test_case = tcase_create("rs_pop");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_pop_ok);
  tcase_add_test(test_case, test_rs_pop_bad);
  
  /* Build rs_contains test cases */
  test_case = tcase_create("rs_contains");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_contains_ok);
  tcase_add_test(test_case, test_rs_contains_bad);

  /* Build rs_size test cases */
  test_case = tcase_create("rs_size");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_size_ok);
  tcase_add_test(test_case, test_rs_size_bad);
  
  /* Build rs_is_empty test cases */
  test_case = tcase_create("rs_is_empty");
  tcase_add_checked_fixture(test_case, test_rs_setup, test_rs_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_rs_is_empty_ok);
  tcase_add_test(test_case, test_rs_is_empty_bad);

  return suite;

}
