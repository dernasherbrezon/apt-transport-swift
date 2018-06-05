#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 201410L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SwiftClient.h"
#include "common.h"

static size_t swift_client_read_auth_header(char *buffer, size_t size, size_t nitems, void *userdata) {
	size_t length = size * nitems;
	char *header = malloc(length + 1);
	memcpy(header, buffer, length);
	header[length] = '\0';
	char *token = cutPrefix(header, "X-Subject-Token: ");
	if (token != NULL) {
		char **response = (char **) userdata;
		*response = token;
	}
	free(header);
	return length;
}

static size_t swift_client_read_auth_response(void *ptr, size_t size, size_t nmemb, void *userp) {
	char **wt = (char **) userp;
	size_t newLength = 0;
	size_t oldLength = 0;
	if ((*wt) != NULL) {
		oldLength = strlen(*wt);
	}
	newLength = oldLength + size * nmemb;

	*wt = realloc(*wt, newLength + 1);
	memcpy((*wt) + oldLength, ptr, size * nmemb);
	(*wt)[newLength] = '\0';

	return size * nmemb;
}

static size_t swift_client_read_auth_request(void *dest, size_t size, size_t nmemb, void *userp) {
	char *body = (char*) userp;
	int allocatedBytes = (sizeof(char) * strlen(body));
	memcpy(dest, body, allocatedBytes);
	return allocatedBytes;
}

const char * swift_client_authenticate(struct SwiftClient* client, struct ContainerConfiguration *configuration) {
	const char *path = "/v3/auth/tokens";
	size_t length = (strlen(configuration->url) + strlen(path));
	char *authUrl = malloc(length + 1);
	if (authUrl == NULL) {
		return "unable to allocate memory";
	}
	strncpy(authUrl, configuration->url, strlen(configuration->url));
	authUrl[strlen(configuration->url)] = '\0';
	strcat(authUrl, path);
	authUrl[length] = '\0';

	curl_easy_setopt(client->curl, CURLOPT_URL, authUrl);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "Expect:");

	const char *template = "{\"auth\":{\"identity\":{\"methods\":[\"password\"],\"password\":{\"user\":{\"name\": \"%s\",\"domain\":{\"name\":\"Default\"},\"password\":\"%s\"}}}}}";
	size_t bodyLength = strlen(template) + strlen(configuration->username) + strlen(configuration->password);
	char *requestBody = malloc(sizeof(char) * (bodyLength + 1));
	int allocatedBytes = snprintf(requestBody, bodyLength, template, configuration->username, configuration->password);
	if (allocatedBytes < 0) {
		return "unable to allocate body";
	}
	requestBody[bodyLength] = '\0';

	curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(client->curl, CURLOPT_READFUNCTION, swift_client_read_auth_request);
	curl_easy_setopt(client->curl, CURLOPT_READDATA, requestBody);
	curl_easy_setopt(client->curl, CURLOPT_POSTFIELDSIZE, allocatedBytes);
	curl_easy_setopt(client->curl, CURLOPT_POST, 1);

	char *responseBody = NULL;

	curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, swift_client_read_auth_response);
	curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, &responseBody);

	char *token = NULL;
	curl_easy_setopt(client->curl, CURLOPT_HEADERFUNCTION, swift_client_read_auth_header);
	curl_easy_setopt(client->curl, CURLOPT_HEADERDATA, &token);

	CURLcode res = curl_easy_perform(client->curl);

	if (res != CURLE_OK) {
		free(responseBody);
		return curl_easy_strerror(res);
	}

	long response_code;
	curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 201) {
		return "Invalid response code";
	}

	client->token = token;

	curl_slist_free_all(headers);
	free(authUrl);
	free(responseBody);
	free(requestBody);
	return NULL;
}

struct SwiftResponse* swift_client_download(struct SwiftClient *client, char *path, char *filename) {
	//FIXME
	return NULL;
}

bool swift_client_push_back(struct SwiftClients **fubar, struct SwiftClient* value) {
	size_t x = *fubar ? fubar[0]->count : 0, y = x + 1;

	if ((x & y) == 0) {
		void *temp = realloc(*fubar, sizeof **fubar + (x + y) * sizeof fubar[0]->value[0]);
		if (!temp) {
			return false;
		}
		*fubar = temp;
	}

	fubar[0]->value[x] = value;
	fubar[0]->count = y;
	return true;
}

struct SwiftClient* swift_client_find(struct SwiftClients **clients, char *container) {
	if (clients == NULL) {
		return NULL;
	}
	for (int i = 0; i < (*clients)->count; i++) {
		struct SwiftClient *cur = (*clients)->value[i];
		if (strncmp(cur->container, container, strlen(container)) == 0) {
			return cur;
		}
	}
	return NULL;
}

void swift_client_client_free(struct SwiftClient *client) {
	if (client == NULL) {
		return;
	}
	free(client->container);
	curl_easy_cleanup(client->curl);
	free(client->token);
	free(client->endpointUrl);
	free(client);
}

struct SwiftClient* swift_client_create(struct SwiftClients **clients, char *container, struct Configuration *config) {
	CURL *curl = curl_easy_init();
	if (!curl) {
		return NULL;
	}

	if (config->verbose) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	}
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "apt-transport-swift");
//	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swift_client_write_data);
	if (config->proxyHostPort != NULL) {
		curl_easy_setopt(curl, CURLOPT_PROXY, config->proxyHostPort);
	}

	struct SwiftClient *result = malloc(sizeof(struct SwiftClient));
	if (result == NULL) {
		curl_easy_cleanup(curl);
		return NULL;
	}
	result->container = strdup(container);
	result->curl = curl;
	result->token = NULL;
	result->endpointUrl = NULL;
	if (!swift_client_push_back(clients, result)) {
		swift_client_client_free(result);
		return NULL;
	}
	return result;
}

void swift_client_response_free(struct SwiftResponse *response) {
	if (response == NULL) {
		return;
	}
	free(response->response_message);
	free(response);
}

void swift_client_clients_free(struct SwiftClients *clients) {
	if (clients == NULL) {
		return;
	}
	for (int i = 0; i < clients->count; i++) {
		swift_client_client_free(clients->value[i]);
	}
	free(clients);
}
