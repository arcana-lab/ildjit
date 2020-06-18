
/* For NULL */
#include <stdlib.h>

/* For test functions */
#include <iljit_gc_copy_test.h>

/* Module to test */
#include <iljit_gc_copy.h>

/* Managed memory size */
#define HEAP_SIZE 16 * sizeof(JITNUINT)

/* Callbacks and properties */
t_gc_behavior behavior;

/* Prototype of stub functions */
JITNUINT test_object_size_zero(void* object);
XanList* test_object_children_null(void* object);
XanList* test_get_root_set_null(void);

/*
 * Setup test environment. Not all routine of iljit_gc_copy module are tested
 * because most of them are aliases of internal routines. Theses routines are
 * tested in modules specific test. Here there is a list of "untested"
 * routines.
 *
 * gc_alloc_object
 * gc_collect
 *
 */
void test_gc_setup(void) {

  /* Setup callbacks */
  behavior.sizeObject = test_object_size_zero;
  behavior.getReferencedObject = test_object_children_null;
  behavior.getRootSet = test_get_root_set_null;

  /* Init garbage collector */
  plugin_interface.init(&behavior, HEAP_SIZE);

}

/* Clean test environment */
void test_gc_teardown(void) {

  plugin_interface.shutdown();

}

/* Get object size */
JITNUINT test_object_size_zero(void* object) {

  return 0;

}

/* Get object children */
XanList* test_object_children_null(void* object) {

  return NULL;

}

/* Get root set */
XanList* test_get_root_set_null(void) {

  return NULL;

}

/* Test gc_init with right values */
START_TEST(test_gc_init_ok) {

  /* Right memory allocation */
  fail_unless(gcm_size(&heap) == HEAP_SIZE);

  /* Callbacks setup */
  fail_unless(object_size == test_object_size_zero);
  fail_unless(object_children == test_object_children_null);
  fail_unless(get_root_set == test_get_root_set_null);

  /* Redo initialization to read return value */
  plugin_interface.shutdown();
  fail_unless(plugin_interface.init(&behavior, HEAP_SIZE) == GC_NO_ERROR);

} END_TEST

/* Test gc_init with bad values */
START_TEST(test_gc_init_bad) {

  /* Partially reset test environment */
  plugin_interface.shutdown();

  /* Try to pass bad sizeObject pointer */
  behavior.sizeObject = NULL;
  fail_unless(plugin_interface.init(&behavior, HEAP_SIZE) == GC_BAD_IFACE);

  /* I need to initialize memory and then shutdown garbage collector */
  gcm_init(&heap, HEAP_SIZE);
  plugin_interface.shutdown();

  /* Same as above, but getReferencedObject is wrong */
  behavior.sizeObject = test_object_size_zero;
  behavior.getReferencedObject = NULL;
  fail_unless(plugin_interface.init(&behavior, HEAP_SIZE) == GC_BAD_IFACE);

  /* Same as above, but shutdown is done in test destructor */
  gcm_init(&heap, HEAP_SIZE);

} END_TEST

/* Test gc_shutdown with right values */
START_TEST(test_gc_shutdown_ok) {

  /* I don't know how to check if memory has been released */

} END_TEST

/* Test gc_shutdown with bad values */
START_TEST(test_gc_shutdown_bad) {

  /* I don't know any usefull test */

} END_TEST

/* Test gc_get_support with good values */
START_TEST(test_gc_get_support_ok) {

  fail_unless(plugin_interface.getSupport() == ILDJIT_GCSUPPORT_ROOTSET);

} END_TEST

/* Test gc_get_support with wrong values */
START_TEST(test_gc_get_support_bad) {

  /* No usefull tests */

} END_TEST

/* Test gc_get_name with right values */
START_TEST(test_gc_get_name_ok) {

  fail_unless(plugin_interface.getName() == GC_NAME);

} END_TEST

/* Test gc_get_name with bad values */
START_TEST(test_gc_get_name_bad) {

  /* No usefull test */

} END_TEST

/* Test gc_get_version with right values */
START_TEST(test_gc_get_version_ok) {

  fail_unless(plugin_interface.getVersion() == GC_VERSION);

} END_TEST

/* Test gc_get_version with wrong values */
START_TEST(test_gc_get_version_bad) {

  /* No usefull test */

} END_TEST

/* Test gc_get_author with good values */
START_TEST(test_gc_get_author_ok) {

  fail_unless(plugin_interface.getAuthor() == GC_AUTHOR);
	
} END_TEST

/* Test gc_get_author with bad values */
START_TEST(test_gc_get_author_bad) {

  /* No usefull test */

} END_TEST

/* Build a suite with all iljit_gc_copy related tests */
Suite* iljit_gc_copy_suite(void) {

  Suite* suite;
  TCase* test_case;

  /* Build suite */
  suite = suite_create("iljit_gc_copy");

  /* Build a test for gc_init */
  test_case = tcase_create("gc_init");
  suite_add_tcase(suite, test_case);
  tcase_add_checked_fixture(test_case, test_gc_setup, test_gc_teardown);
  tcase_add_test(test_case, test_gc_init_ok);
  tcase_add_test(test_case, test_gc_init_bad);

  /* Build a test for gc_shutdown */
  test_case = tcase_create("gc_shutdown");
  suite_add_tcase(suite, test_case);
  tcase_add_checked_fixture(test_case, test_gc_setup, test_gc_teardown);
  tcase_add_test(test_case, test_gc_shutdown_ok);
  tcase_add_test(test_case, test_gc_shutdown_bad);

  /* Build a test for gc_get_support */
  test_case = tcase_create("gc_get_support");
  suite_add_tcase(suite, test_case);
  tcase_add_checked_fixture(test_case, test_gc_setup, test_gc_teardown);
  tcase_add_test(test_case, test_gc_get_support_ok);
  tcase_add_test(test_case, test_gc_get_support_bad);

  /* Build a test for gc_get_name */
  test_case = tcase_create("gc_get_name");
  suite_add_tcase(suite, test_case);
  tcase_add_checked_fixture(test_case, test_gc_setup, test_gc_teardown);
  tcase_add_test(test_case, test_gc_get_name_ok);
  tcase_add_test(test_case, test_gc_get_name_bad);

  /* Build a test for gc_get_version */
  test_case = tcase_create("gc_get_version");
  suite_add_tcase(suite, test_case);
  tcase_add_checked_fixture(test_case, test_gc_setup, test_gc_teardown);
  tcase_add_test(test_case, test_gc_get_version_ok);
  tcase_add_test(test_case, test_gc_get_version_bad);

  /* Build a test for gc_get_author */
  test_case = tcase_create("gc_get_author");
  suite_add_tcase(suite, test_case);
  tcase_add_checked_fixture(test_case, test_gc_setup, test_gc_teardown);
  tcase_add_test(test_case, test_gc_get_author_ok);
  tcase_add_test(test_case, test_gc_get_author_bad);

  return suite;

}
