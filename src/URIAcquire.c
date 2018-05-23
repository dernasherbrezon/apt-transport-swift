#include <stdio.h>
#include <stdlib.h>

#include "URIAcquire.h"

struct URIAcquire* swift_uri_acquire_read() {
	char *line = NULL;
	size_t len = 0;
	ssize_t readBytes;
	struct URIAcquire* result = malloc(sizeof(struct URIAcquire));
	if(result == NULL) {
		return NULL;
	}
	result->expectedMd5 = false;
	result->expectedSha1 = false;
	result->expectedSha256 = false;
	result->expectedSha512 = false;
	while (true) {
		if ((readBytes = getline(&line, &len, stdin)) == -1
				|| (strcmp(line, "\n") == 0)) {
			break;
		}
		if (startsWith(line, "URI: ")) {
			result->uri = substring(line, 5);
		} else if (startsWith(line, "Filename: ")) {
			result->filename = substring(line, 10);
		} else if (startsWith(line, "Expected-MD5Sum")) {
			result->expectedMd5 = true;
		} else if (startsWith(line, "Expected-SHA1")) {
			result->expectedSha1 = true;
		} else if (startsWith(line, "Expected-SHA256")) {
			result->expectedSha256 = true;
		} else if (startsWith(line, "Expected-SHA512")) {
			result->expectedSha512 = true;
		}
	}
	if (result->uri == NULL) {
		fprintf(stderr, "invalid URIAcquire request. Expected URI");
		return NULL;
	}
	if (result->filename == NULL) {
		fprintf(stderr, "invalid URIAcquire request. Expected Filename");
		return NULL;
	}
	return result;
}
