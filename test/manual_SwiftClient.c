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
	const char *containerName = "container";
	struct ContainerConfiguration* containerConfig = malloc(sizeof(struct ContainerConfiguration));
	containerConfig->container = strdup(containerName);
	containerConfig->id = "0";
	containerConfig->url = argv[1];
	containerConfig->username = argv[2];
	containerConfig->password = argv[3];

	printf("initializing...\n");
	printf("url: %s\n", containerConfig->url);
	printf("username: %s\n", containerConfig->username);

	struct Configuration *config = malloc(sizeof(struct Configuration));
	config->verbose = true;
	config->proxyHostPort = NULL;
	config->containers = malloc(sizeof(struct ListOfConfigurations));
	config->containers->count = 1;
//	config->containers->value = malloc(sizeof(struct ContainerConfiguration) * 1);
	config->containers->value[0] = containerConfig;

	struct SwiftClients *clients = NULL;
	struct SwiftClient *client = swift_client_create(&clients, strdup(containerName), config);
	const char *response = swift_client_authenticate(client, containerConfig);
	printf("result: %s", response);

	swift_configuration_free(config);
	swift_client_clients_free(clients);

	return EXIT_SUCCESS;
}

