#include <stdlib.h>
#include <check.h>
#include "../src/common.h"

START_TEST (test_cutPrefix) {
	char *result = cutPrefix("Config-Item: Acquire", "Config-Item: ");
	ck_assert_str_eq(result, "Acquire");
	free(result);
}
END_TEST

START_TEST (test_startsWith) {
	ck_assert(startsWith("Config-Item: Acquire::https::Proxy=http://proxy:80", "Config-Item:"));
}
END_TEST

void assertTrim(const char *str, const char* expected) {
	size_t len = strlen(str);
	char *mutable = malloc(len + 1);
	strcpy(mutable, str);
	mutable[len] = '\0';
	ck_assert_str_eq(trim(mutable), expected);
	free(mutable);
}

START_TEST (test_trim) {
	assertTrim("nothing to trim", "nothing to trim");
	assertTrim("    trim the front", "trim the front");
	assertTrim("trim the back     ", "trim the back");
	assertTrim(" trim one char front and back ", "trim one char front and back");
	assertTrim("                   ", "");
	assertTrim(" ", "");
	assertTrim("a", "a");
	assertTrim("\0", "");
	assertTrim("", "");
	ck_assert(trim(NULL) == NULL);
}
END_TEST

START_TEST (test_concat) {
	const char* actual = concat("api.example.com", "/v3/auth/tokens");
	ck_assert_str_eq("api.example.com/v3/auth/tokens", actual);
	free(actual);
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
	tcase_add_test(tc_core, test_trim);
	tcase_add_test(tc_core, test_concat);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void) {
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = common_suite();
	sr = srunner_create(s);

	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
