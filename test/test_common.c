#include <stdlib.h>
#include <check.h>
#include "../src/common.h"

START_TEST (test_cutPrefix) {
	ck_assert_str_eq(cutPrefix("Config-Item: Acquire::https::Proxy=http://proxy:80", "Config-Item: Acquire::https::Proxy="), "http://proxy:80");
}
END_TEST

START_TEST (test_startsWith) {
	ck_assert(startsWith("Config-Item: Acquire::https::Proxy=http://proxy:80", "Config-Item:"));
}
END_TEST

START_TEST (test_substring) {
	ck_assert_str_eq(substring("Config-Item: Acquire::https::Proxy=http://proxy:80", 13) , "Acquire::https::Proxy=http://proxy:80");
}
END_TEST

Suite * common_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("common");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_cutPrefix);
	tcase_add_test(tc_core, test_startsWith);
	tcase_add_test(tc_core, test_substring);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void) {
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = common_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
