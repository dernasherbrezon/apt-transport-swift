#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "Configuration.h"

struct Configuration* swift_configuration_read() {
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
		if ((readBytes = getline(&line, &len, stdin)) == -1
				|| (strcmp(line, "\n") == 0)) {
			break;
		}

		if (startsWith(line, "Acquire::https::Proxy=")) {
			char * fullUrl = substring(line, 22);
			if (startsWith(fullUrl, "@")) {
				//FIXME
			} else {
				result->proxyHostPort = fullUrl;
			}
		}
	}
	return result;
}
