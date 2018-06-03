#include <stdlib.h>
#include <string.h>
#include "SwiftClient.h"

struct AuthResponse {
	char *body;
};

static size_t swift_client_write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
	return written;
}

static size_t swift_client_read_auth_response(void *ptr, size_t size, size_t nmemb, void *userp) {
	struct AuthResponse *wt = (struct AuthResponse *) userp;
	size_t newLength = 0;
	size_t oldLength = 0;
	if (wt->body != NULL) {
		oldLength = strlen(wt->body);
	}
	newLength = oldLength + size * nmemb;

	wt->body = realloc(wt->body, newLength + 1);
	memcpy(wt->body + oldLength, ptr, size * nmemb);
	wt->body[newLength] = '\0';

	return size * nmemb;
}

static size_t swift_client_read_auth_request(void *dest, size_t size, size_t nmemb, void *userp) {
	struct ContainerConfiguration *wt = (struct ContainerConfiguration *) userp;
	char *body = NULL;
	int allocatedBytes = asprintf(&body, "{\"auth\":{\"identity\":{\"methods\":[\"password\"],\"password\":{\"user\":{\"name\": \"%s\",\"domain\":{\"name\":\"Default\"},\"password\":\"%s\"}}}}}", wt->username, wt->password);
	if (allocatedBytes < 0) {
		return 0;
	}
	memcpy(dest, body, allocatedBytes);
	return 0;
}

char * swift_client_authenticate(struct SwiftClient* client, struct ContainerConfiguration *configuration) {
	const char *path = "/v3/auth/tokens";
	char *authUrl = malloc(sizeof(char) * (strlen(configuration->url) + strlen(path)));
	strcat(authUrl, path);
	curl_easy_setopt(client->curl, CURLOPT_URL, authUrl);

	struct curl_slist *headers;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(client->curl, CURLOPT_POST, 1);
	curl_easy_setopt(client->curl, CURLOPT_READFUNCTION, swift_client_read_auth_request);
	curl_easy_setopt(client->curl, CURLOPT_READDATA, configuration);

	struct AuthResponse response;
	response.body = NULL;

	curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, swift_client_read_auth_response);
	curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(client->curl);

	free(headers);
	free(authUrl);

	if (res != CURLE_OK) {
		free(response.body);
		return curl_easy_strerror(res);
	}

	long response_code;
	curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 201) {
		return "Invalid response code";
	}

	printf("response: %s", response.body);

	free(response.body);
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
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, swift_client_write_data);
	if (config->proxyHostPort != NULL) {
		curl_easy_setopt(curl, CURLOPT_PROXY, config->proxyHostPort);
	}

	struct SwiftClient *result = malloc(sizeof(struct SwiftClient));
	if (result == NULL) {
		return NULL;
	}
	result->container = strdup(container);
	result->curl = curl;
	result->token = NULL;
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