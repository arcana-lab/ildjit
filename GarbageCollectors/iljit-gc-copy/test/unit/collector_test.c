
/* For NULL */
#include <stdlib.h>

/* For memset */
#include <string.h>

/* For test functions */
#include <collector_test.h>

/* Include collector module definitions */
#include <collector.h>

/* The number of live objects in this test */
#define OBJECTS_COUNT 4

/* Only one type of object: JITNUINT */
#define OBJECT_SIZE sizeof(JITNUINT)

/* Memory size */
#define HEAP_SIZE 32 * OBJECT_SIZE

/* Maximum root set size */
#define REFERENCES_COUNT 6

/* Maximum root set dimension */
#define MAX_ROOT_SET_SIZE 2

/* Object value index in object_values array */
#define A 0
#define B 1
#define C 2
#define D 3

/* Object reference index in references array */
#define R1 0
#define R2 1
#define R3 2
#define R4 3
#define R5 4
#define R6 5

/* Reference index in reference_pointers array */
#define RP1 0
#define RP2 1
#define RP3 2
#define RP4 3
#define RP5 4
#define RP6 5

/* Root reference index, used during root set construction */
#define RR1 RP1
#define RR2 RP2
#define RR3 RP6

/* Object to test */
collector_t collector;

/* Managed memory */
gc_memory_t heap;

/* Reference pointers, for raw root set */
void** reference_pointers[REFERENCES_COUNT];

/* Objects references in online semispaces */
void* references[REFERENCES_COUNT];

/* Objects values */
JITNUINT objects_values[OBJECTS_COUNT];

/* Root set */
XanList* root_set;

/* Defined in library */
extern JITNUINT (*object_size)(void* object);
extern XanList* (*object_children)(void* object);
extern XanList* (*get_root_set)(void);

/* Declare functions used by test_co_setup */
JITNUINT test_object_size(void* object);
XanList* test_object_children(void* object);
XanList* test_get_root_set(void);

/*
 * Initialize test environment. Test heap is showed in the following diagram:
 *
 * +-------+------+---+
 * | alpha | dead |   | First column is heap representation. The table head is
 * +-------+------+---+ heap bottom. Every cell is an object. Every object has
 * | A     | live | 1 | size OBJECT_SIZE. There aren't lacks in heap. For
 * +-------+------+---+ example if alpha object has offset 0, C object has
 * | B     | live | 2 | offset 0 + 5 * OBJECT_SIZE.
 * +-------+------+---+
 * | beta  | dead |   | Second column report object state: live means that
 * +-------+------+---+ object is currently reachable from root set stack, while
 * | gamma | dead |   | dead means that object is no longer reachable from root
 * +-------+------+---+ set stack and now it is garbage to be collected. Note
 * | C     | live | 3 | that live objects are identified by a capitalized
 * +-------+------+---+ letter. Dead objects are named with a greek letter.
 * | delta | dead |   |
 * +-------+------+---+ Third colum report object value.
 * | eta   | dead |   |
 * +-------+------+---+
 * | theta | dead |   |
 * +-------+------+---+
 * | D     | live | 4 |
 * +-------+------+---+
 * | rho   | dead |   |
 * +-------+------+---+
 *
 * The second diagram show references:
 *
 * +------+------+------+    Legend:
 * | #### | #### | #### |
 * +--|---+---|--+--|---+    ####               root reference
 *    |       |     |
 *    +->AAA<-+---+ +->DDD   -, |, +, >         links
 *       | |      |
 *       | +->CCC-+          AAA, BBB, ...      object A, objectB, ...
 *       |
 *       +--->BBB
 *
 * In the diagram there is a root set with three root entries. A is an object
 * with two inner references, C has only one inner reference, while B and D
 * haven't any inner references. Link names are reported in the following
 * table.
 *
 * +------+------+----+
 * | Name | From | To | Note: RR is root reference
 * +------+------+----+
 * | R1   | RR1  | A  |
 * +------+------+----+
 * | R2   | RR2  | A  |
 * +------+------+----+
 * | R3   | A    | B  |
 * +------+------+----+
 * | R4   | A    | C  |
 * +------+------+----+
 * | R5   | C    | A  |
 * +------+------+----+
 * | R6   | RR3  | D  |
 * +------+------+----+
 */
void test_co_setup(void) {

  /* Raw root set */
  void** raw_root_set[MAX_ROOT_SET_SIZE];

  /* Curren object value */
  JITNUINT value;

  /* Build fake memory and root set container */
  gcm_init(&heap, HEAP_SIZE);

  /* Fill in offline space with 0 to get a predectible space content */
  memset(heap.offline_semi_space->bottom, 0, HEAP_SIZE / 2);

  /* Build objects and garbage: first some garbage (alpha) */
  gcm_alloc(&heap, OBJECT_SIZE);

  /* Alloc A */
  value = 1;
  references[R1] = gcm_alloc(&heap, OBJECT_SIZE);
  references[R2] = references[R1];
  references[R5] = references[R1];
  *((JITNUINT*) references[R1]) = value;
  objects_values[A] = value++;

  /* Alloc B */
  references[R3] = gcm_alloc(&heap, OBJECT_SIZE);
  *((JITNUINT*) references[R3]) = value;
  objects_values[B] = value++;

  /* Alloc beta, gamma */
  gcm_alloc(&heap, 2 * OBJECT_SIZE);

  /* Alloc C */
  references[R4] = gcm_alloc(&heap, OBJECT_SIZE);
  *((JITNUINT*) references[R4]) = value;
  objects_values[C] = value++;

  /* Alloc delta, eta and theta */
  gcm_alloc(&heap, 3 * OBJECT_SIZE);

  /* Alloc D */
  references[R6] = gcm_alloc(&heap, OBJECT_SIZE);
  *((JITNUINT*) references[R6]) = value;
  objects_values[D] = value;

  /* Alloc rho */
  gcm_alloc(&heap, OBJECT_SIZE);

  /* Setup links (references) */
  reference_pointers[RP1] = &references[RP1];
  reference_pointers[RP2] = &references[RP2];
  reference_pointers[RP3] = &references[RP3];
  reference_pointers[RP4] = &references[RP4];
  reference_pointers[RP5] = &references[RP5];
  reference_pointers[RP6] = &references[RP6];

  /* Setup root set */
  root_set = xanListNew(xan_alloc, xfree, NULL);
  root_set->insert(root_set, reference_pointers[RR1]);
  root_set->insert(root_set, reference_pointers[RR2]);
  root_set->insert(root_set, reference_pointers[RR3]);

  /* Link functions */
  object_size = test_object_size;
  object_children = test_object_children;
  get_root_set = test_get_root_set;

  /* Build a new collector */
  co_init(&collector, &heap);

}

/* Reset test environment */
void test_co_teardown(void) {

  /* Destroy all data structures */
  root_set->destroyList(root_set);
  co_destroy(&collector);
  gcm_destroy(&heap);

}

/* Used to get object size by garbage collecting routines */
JITNUINT test_object_size(void* object) {

  /* Object size is same and constant for all objects */
  return OBJECT_SIZE;

}

/* Used to get object children by garbage collecting routines */
XanList* test_object_children(void* object) {

  /* Loop index */
  JITNUINT i;

  /* Object founded? */
  JITBOOLEAN found;

  /* List of referece pointers */
  XanList* children;

  /* Search object */
  for(i = 0; i < REFERENCES_COUNT; i++)
    if(references[i] == object) {
      found = TRUE;
      break;
    }

  /* Object not found: abort */
  if(!found)
    fail("Object not found");

  /* Object found */
  children = xanListNew(xan_alloc, xfree, NULL);
  switch(i) {
    /* R1, R2 and R5 points to A: it has two inner references */
    case R1:
    case R2:
    case R5:
      children->append(children, reference_pointers[R3]);
      children->append(children, reference_pointers[R4]);
      break;

    /* R4 points to C that has only one reference */
    case R4:
      children->append(children, reference_pointers[R5]);
      break;

    /* Dead objects plus B and D: do nothing */
  }

  return children;

}

/* Mock to get root set */
XanList* test_get_root_set(void) {

  return root_set;

}

/* Test co_init with right values */
START_TEST(test_co_init_ok) {

  /* At start object table is empty */
  fail_unless(co_objects_count(&collector) == 0);

} END_TEST

/* Test co_init with wrong values */
START_TEST(test_co_init_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test co_destroy with good values */
START_TEST(test_co_destroy_ok) {

  /* I don't know how to check if memory has been released */

} END_TEST

/* Test co_destroy with wrong values */
START_TEST(test_co_destroy_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test co_collect_object with right values */
START_TEST(test_co_collect_object_ok) {

  /* Expected object address in offline semi space */
  void* expected_address;

  /* Collect reference R1 */
  co_collect_object(&collector, reference_pointers[R1]);

  /* A must be moved on offline semispace bottom */
  expected_address = heap.offline_semi_space->bottom;
  fail_unless(*(JITNUINT*) references[R1] == objects_values[A]);
  fail_unless(references[R1] == expected_address);

} END_TEST

/* Test co_collect_object with wrong values */
START_TEST(test_co_collect_object_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test co_collect with right values */
START_TEST(test_co_collect_ok) {

  /* Online semi space address before collection */
  void* old_online_semi_space;

  /* Offline semi space address before collection */
  void* old_offline_semi_space;

  /* Object expected address after collection */
  void* expected_address;

  /* Save semi spaces info and run collection algorithm */
  old_online_semi_space = heap.online_semi_space;
  old_offline_semi_space = heap.offline_semi_space;
  co_collect(&collector);

  /* Semispaces must be swapped */
  fail_unless(heap.online_semi_space == old_offline_semi_space);
  fail_unless(heap.offline_semi_space == old_online_semi_space);

  /* First object visited is A */
  expected_address = heap.online_semi_space->bottom;
  fail_unless(*(JITNUINT*) references[R1] == objects_values[A]);
  fail_unless(references[R1] == expected_address);

  /* C is moved on second position */
  expected_address += OBJECT_SIZE;
  fail_unless(*(JITNUINT*) references[R4] == objects_values[C]);
  fail_unless(references[R4] == expected_address);

  /* B on third position */
  expected_address += OBJECT_SIZE;
  fail_unless(*(JITNUINT*) references[R3] == objects_values[B]);
  fail_unless(references[R3] == expected_address);

  /* D at last */
  expected_address += OBJECT_SIZE;
  fail_unless(*(JITNUINT*) references[R6] == objects_values[D]);
  fail_unless(references[R6] == expected_address);
  
  /* Checks for A children are in co_collect_object_ok test */

} END_TEST

/* Test co_collect with wrong values */
START_TEST(test_co_collect_bad) {

  /* No way to check good assertion fails */

} END_TEST

/* Test co_mark_collected with right values */
START_TEST(test_co_mark_collected_ok) {

  /* Object old address */
  void* old_address;

  /* Object new address */
  void* new_address;

  /* co_mark_collected exit value */
  JITNUINT retval;

  /* Init pointers */
  old_address = heap.online_semi_space->bottom;
  new_address = heap.offline_semi_space->bottom;

  /* Try marking an object */
  retval = co_mark_collected(&collector, old_address, new_address);
  fail_unless(retval == CO_ALL_OK);
  fail_unless(co_lookup_object(&collector, old_address) == new_address);

  /* Mark another time the same object */
  retval = co_mark_collected(&collector, old_address, new_address);
  fail_unless(retval == CO_DOUBLE_MARK);
  fail_unless(co_lookup_object(&collector, old_address) == new_address);

} END_TEST

/* Test co_mark_collected with bad values */
START_TEST(test_co_mark_collected_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Test co_lookup_object with right values */
START_TEST(test_co_lookup_object_ok) {

  /* Object old address */
  void* old_address;

  /* Object new address */
  void* new_address;

  /* Init pointers */
  old_address = heap.online_semi_space->bottom;
  new_address = heap.online_semi_space->bottom;

  /* Mark an object and try get new address */
  co_mark_collected(&collector, old_address, new_address);
  fail_unless(co_lookup_object(&collector, old_address) == new_address);

} END_TEST

/* Test co_lookup_objectwith bad values */
START_TEST(test_co_lookup_object_bad) {

  /* Fake object address */
  void* address;

  /* Init object address */
  address = heap.online_semi_space->bottom;

  /* Try search an unknown object */
  fail_unless(co_lookup_object(&collector, address) == NULL);

} END_TEST

/* Test co_marked with right values */
START_TEST(test_co_marked_ok) {

  /* Object offset from semi space bottom */
  JITNUINT offset;

  /* Online semi space bottom */
  void* bottom;

  /* Init pointer */
  bottom = heap.online_semi_space->bottom;

  /* At start no one objects is marked */
  for(offset = 0; offset < OBJECT_SIZE * OBJECTS_COUNT; offset += OBJECT_SIZE)
    fail_unless(co_marked(&collector, bottom + offset) == FALSE);

  /* Mark and object and test */
  co_mark_collected(&collector, bottom, heap.offline_semi_space->bottom);
  fail_unless(co_marked(&collector, bottom));

} END_TEST

/* Test co_marked with wrong values */
START_TEST(test_co_marked_bad) {

  /* No way to check "good" assertions fails */

} END_TEST

/* Test co_objects_count with right values */
START_TEST(test_co_objects_count_ok) {

  /* Object offset from semispace bottom */
  JITNUINT offset;

  /* At start 0 objects has been visited */
  fail_unless(co_objects_count(&collector) == 0);

  /* Mark all online semispace objects as visited: suppose memory compressed */
  for(offset = 0; offset < OBJECTS_COUNT * OBJECT_SIZE; offset += OBJECT_SIZE)
    co_mark_collected(&collector, heap.online_semi_space->bottom + offset,
		                  heap.offline_semi_space->bottom + offset);
  
  /* When must have visited all objects */
  fail_unless(co_objects_count(&collector) == OBJECTS_COUNT);

} END_TEST

/* Test co_objects_count with bad values */
START_TEST(test_co_objects_count_bad) {

  /* No way to check "good" assertion fails */

} END_TEST

/* Build a suite with all collector related tests */
Suite* collector_suite(void) {

  Suite* suite;
  TCase* test_case;

  /* Build suite */
  suite = suite_create("collector");

  /* Build a test for co_init */
  test_case = tcase_create("co_init");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_init_ok);
  tcase_add_test(test_case, test_co_init_bad);

  /* Build a test for co_destroy */
  test_case = tcase_create("co_destroy");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_destroy_ok);
  tcase_add_test(test_case, test_co_destroy_bad);

  /* Build a test for co_collect_object */
  test_case = tcase_create("co_collect_object");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_collect_object_ok);
  tcase_add_test(test_case, test_co_collect_object_bad);

  /* Build a test for co_collect */
  test_case = tcase_create("co_collect");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_collect_ok);
  tcase_add_test(test_case, test_co_collect_bad);

  /* Build a test for co_mark_collected */
  test_case = tcase_create("co_mark_collected");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_mark_collected_ok);
  tcase_add_test(test_case, test_co_mark_collected_bad);

  /* Build a test for co_lookup_object */
  test_case = tcase_create("co_lookup_object");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_lookup_object_ok);
  tcase_add_test(test_case, test_co_lookup_object_bad);

  /* Build a test for co_marked */
  test_case = tcase_create("co_marked");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_marked_ok);
  tcase_add_test(test_case, test_co_marked_bad);

  /* Build a test for co_objects_count */
  test_case = tcase_create("co_objects_count");
  tcase_add_checked_fixture(test_case, test_co_setup, test_co_teardown);
  suite_add_tcase(suite, test_case);
  tcase_add_test(test_case, test_co_objects_count_ok);
  tcase_add_test(test_case, test_co_objects_count_bad);

  return suite;

}
