#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../src/SwiftClient.h"
#include "../src/Configuration.h"
#include "../src/URIAcquire.h"

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("expected: url login password\n");
		return 1;
	}
	char *containerName = "stopcrawl";
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
	struct SwiftClient *client = NULL;
	client = swift_client_create(&clients, containerName, config);
	const char *response = swift_client_authenticate(client, containerConfig);
	if (response != NULL) {
		printf("result: %s\n", response);
	}

	printf("token: %s\n", client->token);
	printf("endpoint: %s\n", client->endpointUrl);

	struct URIAcquire* message = malloc(sizeof(struct URIAcquire));
	if (message == NULL) {
		return 1;
	}

	message->container = NULL;
	message->uri = NULL;
	message->path = strdup("/1/crawlers/crawlers.ser");
	message->filename = strdup("test");
	//message->lastModified = NULL;
	message->lastModified = strdup("Thu, 15 Sep 2016 00:00:02 GMT"); //arbitary future date

	struct SwiftResponse* downloadResponse = swift_client_download(client, message);
	if (downloadResponse != NULL) {
		printf("code: %ld\n", downloadResponse->response_code);
		printf("message: %s\n", downloadResponse->response_message);
	}

	swift_uri_acquire_free(message);
	swift_configuration_free(config);
	swift_client_clients_free(clients);
	swift_client_response_free(downloadResponse);

	return EXIT_SUCCESS;
}

