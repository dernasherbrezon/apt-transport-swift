#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/URIAcquire.h"

START_TEST (test_readmessage) {
	FILE *fp;
	fp = fopen("uri_acquire.txt", "r");
	ck_assert(fp != NULL);
	struct URIAcquire* message = swift_uri_acquire_read(fp);
	fclose(fp);
	ck_assert_ptr_nonnull(message);
	ck_assert(message->expectedMd5 == false);
	ck_assert(message->expectedSha1 == true);
	ck_assert(message->expectedSha256 == false);
	ck_assert(message->expectedSha512 == false);
	ck_assert_str_eq(message->filename, "dists_stretch_InRelease");
	ck_assert_str_eq(message->uri, "swift://container/dists/stretch/InRelease");
	ck_assert_str_eq(message->container, "container");
	ck_assert_str_eq(message->path, "/dists/stretch/InRelease");
	ck_assert_str_eq(message->lastModified, "Wed, 23 May 2018 14:13:16 GMT");
	free(message);
}
END_TEST

Suite * common_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("URIAcquire");

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
