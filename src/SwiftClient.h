#ifndef SWIFTCLIENT_H_
#define SWIFTCLIENT_H_

#include <curl/curl.h>
#include <stdbool.h>
#include "Configuration.h"
#include "URIAcquire.h"

struct SwiftClient {
	CURL *curl;
	char *token;
	char *container;
	char *endpointUrl; //differs from baseUrl. Example: https://api.selcdn.ru/v1/account.
};

struct SwiftClients {
	size_t count;
	struct SwiftClient *value[];
};

struct SwiftResponse {
	long response_code;
	char *response_message;
};

struct SwiftClient* swift_client_find(struct SwiftClients **clients, char *container);

struct SwiftClient* swift_client_create(struct SwiftClients **clients, char *container, struct Configuration *config);

const char* swift_client_authenticate(struct SwiftClient* client, struct ContainerConfiguration *configuration);

struct SwiftResponse* swift_client_download(struct SwiftClient *client, struct URIAcquire* message);

void swift_client_clients_free(struct SwiftClients *clients);

void swift_client_response_free(struct SwiftResponse *response);

#endif
