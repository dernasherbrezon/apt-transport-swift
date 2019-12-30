#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 201410L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>

#include "SwiftClient.h"
#include "common.h"

static size_t swift_client_read_auth_header(char *buffer, size_t size, size_t nitems, void *userdata) {
	size_t length = size * nitems;
	char *header = malloc(length + 1);
	if (header == NULL) {
		//can't do much
		return length;
	}
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
	if (*wt == NULL) {
		//can't do much
		return size * nmemb;
	}
	memcpy((*wt) + oldLength, ptr, size * nmemb);
	(*wt)[newLength] = '\0';

	return size * nmemb;
}

struct Request {
	char *body;
	size_t wrote;
};

static size_t swift_client_read_auth_request(void *dest, size_t size, size_t nmemb, void *userp) {
	struct Request* request = (struct Request*) userp;
	if (request->wrote != 0) {
		//request->wrote could be boolean
		return 0;
	}
	int allocatedBytes = (sizeof(char) * strlen(request->body));
	request->wrote = allocatedBytes;
	memcpy(dest, request->body, allocatedBytes);
	return allocatedBytes;
}

char* swift_client_extract_endpoint(char *body) {
	json_object *obj;
	obj = json_tokener_parse(body);
	json_object *token;
	if (!json_object_object_get_ex(obj, "token", &token)) {
		json_object_put(obj);
		return NULL;
	}
	json_object *catalog;
	if (!json_object_object_get_ex(token, "catalog", &catalog)) {
		json_object_put(obj);
		return NULL;
	}
	json_object *first = json_object_array_get_idx(catalog, 0);
	if (first == NULL) {
		json_object_put(obj);
		return NULL;
	}
	json_object *endpoints;
	if (!json_object_object_get_ex(first, "endpoints", &endpoints)) {
		json_object_put(obj);
		return NULL;
	}
	array_list *list = json_object_get_array(endpoints);
	for (int i = 0; i < list->length; i++) {
		json_object *cur = json_object_array_get_idx(endpoints, i);
		json_object *interface;
		json_object_object_get_ex(cur, "interface", &interface);
		if (strcmp(json_object_get_string(interface), "public") == 0) {
			json_object *url;
			if (!json_object_object_get_ex(cur, "url", &url)) {
				continue;
			}
			char *result = strdup(json_object_get_string(url));
			json_object_put(obj);
			return result;
		}
	}
	json_object_put(obj);
	return NULL;
}

const char * swift_client_authenticate(struct SwiftClient* client, struct ContainerConfiguration *configuration) {
	char *authUrl = concat(configuration->url, "/v3/auth/tokens");
	if (authUrl == NULL) {
		return "unable to allocate memory";
	}
	curl_easy_setopt(client->curl, CURLOPT_URL, authUrl);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "Expect:");

	const char *template = "{\"auth\":{\"identity\":{\"methods\":[\"password\"],\"password\":{\"user\":{\"name\": \"%s\",\"domain\":{\"name\":\"Default\"},\"password\":\"%s\"}}}}}";
	//4 - is for 2 %s
	size_t requestLength = (strlen(template) - 4) + strlen(configuration->username) + strlen(configuration->password);
	char *requestBody = malloc(sizeof(char) * (requestLength + 1));
	if (requestBody == NULL) {
		return "unable to allocate body";
	}
	int allocatedBytes = snprintf(requestBody, requestLength + 1, template, configuration->username, configuration->password);
	requestBody[requestLength] = '\0';

	struct Request request;
	request.body = requestBody;
	request.wrote = 0;

	curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(client->curl, CURLOPT_READFUNCTION, swift_client_read_auth_request);
	curl_easy_setopt(client->curl, CURLOPT_READDATA, &request);
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
		free(token);
		return curl_easy_strerror(res);
	}

	long response_code;
	curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 201) {
		free(responseBody);
		free(token);
		return "Invalid response code";
	}

	client->endpointUrl = swift_client_extract_endpoint(responseBody);
	client->token = token;

	curl_slist_free_all(headers);
	free(authUrl);
	free(responseBody);
	free(requestBody);

	if (client->endpointUrl == NULL || client->token == NULL) {
		return "unable to authenticate";
	}
	return NULL;
}

struct SwiftResponse* swift_client_download(struct SwiftClient *client, struct URIAcquire* message) {

	struct SwiftResponse* result = malloc(sizeof(struct SwiftResponse));
	if (result == NULL) {
		return NULL;
	}

	const char *template = "%s/%s%s";
	//1 is for / in template
	size_t requestLength = (strlen(client->endpointUrl) + 1 + strlen(client->container) + strlen(message->path));
	char *requestUrl = malloc(sizeof(char) * (requestLength + 1));
	if (requestUrl == NULL) {
		result->response_code = 503;
		result->response_message = strdup("unable to allocate memory");
		return result;
	}
	int allocatedBytes = snprintf(requestUrl, requestLength + 1, template, client->endpointUrl, client->container, message->path);
	requestUrl[requestLength] = '\0';

	curl_easy_setopt(client->curl, CURLOPT_URL, requestUrl);

	char *token = concat("X-Auth-Token: ", client->token);
	if (token == NULL) {
		result->response_code = 503;
		result->response_message = strdup("unable to allocate memory");
		return result;
	}
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, token);

	char *lastModified = NULL;
	if (message->lastModified != NULL) {
		lastModified = concat("If-Modified-Since: ", message->lastModified);
		if (lastModified == NULL) {
			result->response_code = 503;
			result->response_message = strdup("unable to allocate memory");
			return result;
		}
		headers = curl_slist_append(headers, lastModified);
	}

	FILE *pagefile = fopen(message->filename, "wb");
	if (!pagefile) {
		result->response_code = 503;
		result->response_message = strdup("unable to open file for write");
		return result;
	}

	curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, pagefile);
	curl_easy_setopt(client->curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(client->curl, CURLOPT_READDATA, NULL);
	curl_easy_setopt(client->curl, CURLOPT_POST, 0);
	curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, NULL);

	CURLcode res = curl_easy_perform(client->curl);
	fclose(pagefile);
	if (res != CURLE_OK) {
		result->response_code = 503;
		result->response_message = strdup(curl_easy_strerror(res));
	} else {
		long response_code;
		curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response_code);
		result->response_code = response_code;
		result->response_message = NULL;
	}
	curl_slist_free_all(headers);
	free(requestUrl);
	free(token);
	free(lastModified);
	return result;
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
