#include <stdlib.h>

#include "URIAcquire.h"
#include "common.h"

struct URIAcquire* swift_uri_acquire_read(FILE *source) {
	char *line = NULL;
	size_t len = 0;
	ssize_t readBytes;
	struct URIAcquire* result = malloc(sizeof(struct URIAcquire));
	if (result == NULL) {
		return NULL;
	}
	result->lastModified = NULL;
	result->container = NULL;
	result->username = NULL;
	result->expectedMd5 = false;
	result->expectedSha1 = false;
	result->expectedSha256 = false;
	result->expectedSha512 = false;
	while (true) {
		if ((readBytes = getline(&line, &len, source)) == -1
				|| (strcmp(line, "\n") == 0)) {
			break;
		}
		char* uri = cutPrefix(line, "URI: ");
		if (uri != NULL) {
			result->uri = uri;
			continue;
		}
		char *filename = cutPrefix(line, "Filename: ");
		if (filename != NULL) {
			result->filename = filename;
			continue;
		}
		char *lastModified = cutPrefix(line, "Last-Modified: ");
		if (lastModified != NULL) {
			result->lastModified = lastModified;
			continue;
		}
		if (startsWith(line, "Expected-MD5Sum")) {
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
		free(result);
		return NULL;
	}
	if (result->filename == NULL) {
		fprintf(stderr, "invalid URIAcquire request. Expected Filename");
		free(result);
		return NULL;
	}
	return result;
}
