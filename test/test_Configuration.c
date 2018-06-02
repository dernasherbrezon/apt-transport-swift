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
	ck_assert(message != NULL);
	ck_assert_str_eq(message->proxyHostPort, "https://username:password@host:8080");
	ck_assert(message->containers != NULL);
	ck_assert_int_eq(message->containers->count, 2);

	struct ContainerConfiguration *value1 = swift_configuration_find_by_container(message, "test0");
	ck_assert(value1 != NULL);
	ck_assert_str_eq(value1->id, "0");
	ck_assert_str_eq(value1->url, "https://example.com/v1");
	ck_assert_str_eq(value1->container, "test0");
	ck_assert_str_eq(value1->username, "username0");
	ck_assert_str_eq(value1->password, "password0");

	struct ContainerConfiguration *value2 = swift_configuration_find_by_container(message, "test1");
	ck_assert(value2 != NULL);
	ck_assert_str_eq(value2->id, "17");
	ck_assert_str_eq(value2->url, "https://example.com/v3");
	ck_assert_str_eq(value2->container, "test1");
	ck_assert_str_eq(value2->username, "username1");
	ck_assert_str_eq(value2->password, "password1");

	ck_assert(message->verbose == true);

	struct ContainerConfiguration *value3 = swift_configuration_find_by_container(message, "unknown");
	ck_assert(value3 == NULL);

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

	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
