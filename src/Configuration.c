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
	result->proxyAuth = NULL;
	while (true) {
		if ((readBytes = getline(&line, &len, source)) == -1
				|| (strcmp(line, "\n") == 0)) {
			break;
		}
		char *fullUrl = cutPrefix(line, "Config-Item: Acquire::https::Proxy=");
		if (fullUrl != NULL) {
			result->proxyHostPort = fullUrl;
		}
	}
	return result;
}
