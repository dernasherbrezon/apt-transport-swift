#include <stdlib.h>
#include <check.h>
#include "../src/Hashes.h"

START_TEST (test_md5Only) {
	FILE *fp;
	fp = fopen("file-for-hashes.txt", "r");
	ck_assert(fp != NULL);

	struct URIAcquire req;
	req.expectedMd5 = true;
	req.expectedSha1 = false;
	req.expectedSha256 = false;
	req.expectedSha512 = false;

	struct Hashes* hashes = swift_hash_file(&req, fp);
	ck_assert_str_eq(hashes->md5, "26e078b87fdaa3206ab8bf63a6096c07");
	ck_assert(hashes->sha1 == NULL);
	ck_assert(hashes->sha256 == NULL);
	ck_assert(hashes->sha512 == NULL);
	ck_assert_int_eq(hashes->fileSize, 11);
}
END_TEST

START_TEST (test_hashes) {
	FILE *fp;
	fp = fopen("file-for-hashes.txt", "r");
	ck_assert(fp != NULL);

	struct URIAcquire req;
	req.expectedMd5 = true;
	req.expectedSha1 = true;
	req.expectedSha256 = true;
	req.expectedSha512 = true;

	struct Hashes* hashes = swift_hash_file(&req, fp);
	ck_assert_str_eq(hashes->md5, "26e078b87fdaa3206ab8bf63a6096c07");
	ck_assert_str_eq(hashes->sha1, "3579307d55e123bde331b3eefce08090bea3fbe7");
	ck_assert_str_eq(hashes->sha256, "f107aac59dff1d49ebfedb7f03877eaa0297f9a7d3cff26edfc75406f222256d");
	ck_assert_str_eq(hashes->sha512, "334a142ae03fa3e64b5ee6b1ed4e406305d2653567fa874465f749856ba64d3afac7621bc49b62374bd8abb397999911b1848a6ede015d2151af8ff343fa0f60");
	ck_assert_int_eq(hashes->fileSize, 11);
}
END_TEST

Suite * common_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("common");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_hashes);
	tcase_add_test(tc_core, test_md5Only);
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
