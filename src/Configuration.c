#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "Configuration.h"

bool push_back(struct ListOfConfigurations **fubar, struct ContainerConfiguration* value) {
	size_t x = *fubar ? fubar[0]->count : 0, y = x + 1;

	if ((x & y) == 0) {
		void *temp = realloc(*fubar, sizeof **fubar + (x + y) * sizeof fubar[0]->value[0]);
		if (!temp) {
			return false;
		}
		*fubar = temp; // or, if you like, `fubar[0] = temp;`
	}

	fubar[0]->value[x] = value;
	fubar[0]->count = y;
	return true;
}

struct ContainerConfiguration * swift_configuration_find_by_id(struct ListOfConfigurations *fubar, const char* id) {
	if (fubar == NULL) {
		return NULL;
	}
	for (int i = 0; i < fubar->count; i++) {
		struct ContainerConfiguration *cur = fubar->value[i];
		if (strncmp(cur->id, id, strlen(id)) == 0) {
			return cur;
		}
	}
	return NULL;
}

struct ContainerConfiguration* swift_configuration_find_by_container(struct Configuration *config, const char* containerName) {
	if (config == NULL || config->containers == NULL) {
		return NULL;
	}
	for (int i = 0; i < config->containers->count; i++) {
		struct ContainerConfiguration *cur = config->containers->value[i];
		if (strncmp(cur->container, containerName, strlen(containerName)) == 0) {
			return cur;
		}
	}
	return NULL;
}

struct Configuration* swift_configuration_read(FILE* source) {
	char *line = NULL;
	size_t len = 0;
	ssize_t readBytes;
	struct Configuration* result = malloc(sizeof(struct Configuration));
	if (result == NULL) {
		return NULL;
	}
	result->containers = NULL;
	result->proxyHostPort = NULL;
	result->verbose = false;
	while (true) {
		if ((readBytes = getline(&line, &len, source)) == -1 || (strcmp(line, "\n") == 0)) {
			break;
		}
		char *fullUrl = cutPrefix(line, "Config-Item: Acquire::https::Proxy=");
		if (fullUrl != NULL) {
			result->proxyHostPort = fullUrl;
			continue;
		}
		char *container = cutPrefix(line, "Config-Item: Swift::Container");
		if (container != NULL) {
			char *pch;
			pch = strtok(container, ":");
			if (pch == NULL) {
				fprintf(stderr, "invalid format: %s\n", container);
				continue;
			}
			struct ContainerConfiguration * config = swift_configuration_find_by_id(result->containers, pch);
			if (config == NULL) {
				config = malloc(sizeof(struct ContainerConfiguration));
				if (config == NULL) {
					break;
				}
				config->id = pch;
				config->container = NULL;
				config->password = NULL;
				config->url = NULL;
				config->username = NULL;
				if (!push_back(&result->containers, config)) {
					break;
				}
			}
			pch = strtok(NULL, "=");
			if (pch == NULL) {
				fprintf(stderr, "invalid format: %s\n", container);
				continue;
			}
			char *value = strtok(NULL, "=");
			if (value == NULL) {
				fprintf(stderr, "invalid format: %s\n", container);
				continue;
			}
			if (strcmp(pch, ":Name") == 0) {
				config->container = value;
				continue;
			}
			if (strcmp(pch, ":Username") == 0) {
				config->username = value;
				continue;
			}
			if (strcmp(pch, ":Password") == 0) {
				config->password = value;
				continue;
			}
			if (strcmp(pch, ":URL") == 0) {
				config->url = value;
				continue;
			}
			continue;
		}
		if (strcmp(trim(line), "Config-Item: Debug::Acquire::https=true") == 0) {
			result->verbose = true;
			continue;
		}

	}
	// do not support anonymous explicitly
	// use plain https/http transport for anonymous access
	if (result->containers == NULL || result->containers->count == 0) {
		fprintf(stderr, "container is not configured\n");
		free(result);
		return NULL;
	}
	return result;
}
