#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "Configuration.h"

struct Configuration* swift_configuration_read(FILE* source) {
	char *line = NULL;
	size_t len = 0;
	ssize_t readBytes;
	struct Configuration* result = malloc(sizeof(struct Configuration));
	if (result == NULL) {
		return NULL;
	}
	result->proxyHostPort = NULL;
	while (true) {
		if ((readBytes = getline(&line, &len, source)) == -1 || (strcmp(line, "\n") == 0)) {
			break;
		}
		char *fullUrl = cutPrefix(line, "Config-Item: Acquire::https::Proxy=");
		if (fullUrl != NULL) {
			result->proxyHostPort = fullUrl;
			continue;
		}
		char *container = cutPrefix(line, "Config-Item: Swift::Container::Name=");
		if (container != NULL) {
			result->container = container;
			continue;
		}
		char *username = cutPrefix(line, "Config-Item: Swift::Auth::Username=");
		if (username != NULL) {
			result->username = username;
			continue;
		}
		char *password = cutPrefix(line, "Config-Item: Swift::Auth::Password=");
		if (password != NULL) {
			result->password = password;
			continue;
		}
		// do not support anonymous explicitly
		// use plain https/http transport for anonymous access
		if (result->container == NULL || result->username == NULL || result->password == NULL) {
			fprintf(stderr, "access is not configured");
			free(result);
			return NULL;
		}
	}
	return result;
}
