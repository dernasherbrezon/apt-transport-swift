#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/Configuration.h"

START_TEST (test_readmessage) {
	FILE *fp;
	fp = fopen("configuration.txt", "r");
	ck_assert(fp != NULL);
	struct Configuration* message = swift_configuration_read(fp);
	fclose(fp);
	ck_assert(message);
	ck_assert_str_eq(message->proxyHostPort, "https://username:password@host:8080");
	ck_assert_str_eq(message->container, "test");
	ck_assert_str_eq(message->username, "username");
	ck_assert_str_eq(message->password, "password");
	ck_assert(message->verbose == true);
	free(message);
}
END_TEST

Suite * common_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("Configuration");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_readmessage);
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
