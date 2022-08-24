#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdlib.h>
#include <stdio.h>

#include "../src/yaml_reader.h"
#define STRUCTURE_FILE "./data/test_yaml_reader_structure.yml"
#define STR_MAX 100

static void test_yaml_reads_key_names(void **state)
{
  (void) state;
  char **keys = NULL;
  int n_keys = 0;
  int rc;

  rc = yaml_get_keys(STRUCTURE_FILE, &keys, &n_keys, STR_MAX);
  assert_int_equal(rc, 0);
  assert_int_equal(n_keys, 8);
  assert_string_equal(keys[0], "placeholder");
}

static void test_yaml_finds_key_value(void **state)
{
  (void) state;
  char value[STR_MAX];
  int rc;

  rc = yaml_get_map_value(STRUCTURE_FILE, "test_get_value", value, STR_MAX);
  assert_int_equal(rc, 0);
  assert_string_equal(value, "Success");
};

static void test_yaml_errors_on_missing_key(void **state)
{
  (void) state;
  char value[STR_MAX];
  int rc;

  rc = yaml_get_map_value(STRUCTURE_FILE, "test_missing_key", value, STR_MAX);
  assert_int_equal(rc, YAML__ERROR_KEY);
}

static void test_yaml_errors_on_missing_value(void **state)
{
  (void) state;
  char value[STR_MAX];
  int rc;

  rc = yaml_get_map_value(STRUCTURE_FILE, "test_missing_value", value, STR_MAX);
  assert_int_equal(rc, YAML__ERROR_VALUE);
}

static void test_yaml_handles_string_literals(void **state)
{
  /* Should ignore syntax when in '\'' or '"'. */
  (void) state;
  char value[STR_MAX];
  int rc;

  rc = yaml_get_map_value(STRUCTURE_FILE, "test_handles_string_literals", value,
                          STR_MAX);
  assert_int_equal(rc, 0);
  assert_string_equal(value, "/test/this/{path,syntax}");
}

static void test_yaml_errors_on_malformed_value(void **state)
{
  (void) state;
  char value[STR_MAX];
  int rc;

  rc = yaml_get_map_value(STRUCTURE_FILE, "test_malformed_value", value,
                          STR_MAX);
  assert_int_equal(rc, YAML__ERROR_VALUE);
}

static void test_yaml_finds_map_contents_multiline(void **state)
{
  (void) state;
  size_t n_items = 0;
  char **key_value_pairs[2];
  int rc;

  rc = yaml_get_map_contents(STRUCTURE_FILE, "test_get_contents_multiline",
                             key_value_pairs, &n_items);

  char **keys = key_value_pairs[0];
  char **values = key_value_pairs[1];

  assert_int_equal(rc, 0);
  assert_int_equal(n_items, 3);
  assert_string_equal(keys[0], "a");
  assert_string_equal(keys[1], "b");
  assert_string_equal(keys[2], "c");
  assert_string_equal(values[0], "1");
  assert_string_equal(values[1], "2");
  assert_string_equal(values[2], "3");
}

static void test_yaml_finds_map_contents_inline(void **state)
{
  (void) state;
  size_t n_items = 0;
  char **key_value_pairs[2];
  int rc;

  rc = yaml_get_map_contents(STRUCTURE_FILE, "test_get_contents_inline",
                             key_value_pairs, &n_items);

  char **keys = key_value_pairs[0];
  char **values = key_value_pairs[1];

  assert_int_equal(rc, 0);
  assert_int_equal(n_items, 3);
  assert_string_equal(keys[0], "a");
  assert_string_equal(keys[1], "b");
  assert_string_equal(keys[2], "c");
  assert_string_equal(values[0], "1");
  assert_string_equal(values[1], "2");
  assert_string_equal(values[2], "3");
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_yaml_reads_key_names),
    cmocka_unit_test(test_yaml_finds_key_value),
    cmocka_unit_test(test_yaml_errors_on_missing_key),
    cmocka_unit_test(test_yaml_errors_on_missing_value),
    cmocka_unit_test(test_yaml_errors_on_malformed_value),
    cmocka_unit_test(test_yaml_handles_string_literals),
    cmocka_unit_test(test_yaml_finds_map_contents_multiline),
    cmocka_unit_test(test_yaml_finds_map_contents_inline)
  };

  return cmocka_run_group_tests_name("YAML get map tests", tests, NULL, NULL);
}
