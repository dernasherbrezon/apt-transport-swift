#include <stdlib.h>
#include <check.h>

#include "../src/SwiftClient.h"
#include "../src/Configuration.h"

START_TEST (test_init) {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	struct SwiftClients *clients = NULL;
	struct Configuration *config = malloc(sizeof(struct Configuration));
	config->verbose = false;
	config->proxyHostPort = NULL;
	config->containers = NULL;
	struct SwiftClient* client = swift_client_create(&clients, "test", config);
	ck_assert(clients != NULL);
	ck_assert(client != NULL);
	swift_client_clients_free(clients);
	swift_configuration_free(config);
	curl_global_cleanup();
}
END_TEST

Suite * common_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("SwiftClient");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_init);
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



