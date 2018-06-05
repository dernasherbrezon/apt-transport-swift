#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../src/SwiftClient.h"
#include "../src/Configuration.h"

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("expected: url login password\n");
		return 1;
	}
	char *containerName = "container";
	struct ContainerConfiguration* containerConfig = malloc(sizeof(struct ContainerConfiguration));
	containerConfig->container = strdup(containerName);
	containerConfig->id = strdup("0");
	containerConfig->url = strdup(argv[1]);
	containerConfig->username = strdup(argv[2]);
	containerConfig->password = strdup(argv[3]);

	printf("initializing...\n");
	printf("url: %s\n", containerConfig->url);
	printf("username: %s\n", containerConfig->username);

	struct ListOfConfigurations *configs = malloc(sizeof(struct ListOfConfigurations) + 1 * sizeof(struct Configuration));
	if (configs == NULL) {
		return 1;
	}
	struct Configuration *config = malloc(sizeof(struct Configuration));
	if (config == NULL) {
		return 1;
	}
	config->verbose = true;
	config->proxyHostPort = NULL;
	config->containers = configs;
	config->containers->count = 1;
	config->containers->value[0] = containerConfig;

	struct SwiftClients *clients = NULL;
	struct SwiftClient *client = swift_client_create(&clients, containerName, config);
	const char *response = swift_client_authenticate(client, containerConfig);
	if (response != NULL) {
		printf("result: %s\n", response);
	}

	swift_configuration_free(config);
	swift_client_clients_free(clients);

	return EXIT_SUCCESS;
}

