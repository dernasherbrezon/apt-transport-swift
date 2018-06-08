#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 201410L
#endif
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <uriparser/Uri.h>

#include "URIAcquire.h"
#include "common.h"

bool swift_uri_acquire_parse_uri(const char *src, struct URIAcquire* result) {
	UriParserStateA state;
	UriUriA parsedUri;
	state.uri = &parsedUri;
	if (uriParseUriA(&state, src) != URI_SUCCESS) {
		uriFreeUriMembersA(&parsedUri);
		return false;
	}

	size_t len = sizeof(char) * (parsedUri.hostText.afterLast - parsedUri.hostText.first);
	result->container = malloc(len + 1);
	if (result->container == NULL) {
		return false;
	}
	strncpy(result->container, parsedUri.hostText.first, len);
	result->container[len] = '\0';

	const char* lastSegment;
	if (parsedUri.fragment.afterLast != NULL) {
		lastSegment = parsedUri.fragment.afterLast;
	} else if (parsedUri.query.afterLast != NULL) {
		lastSegment = parsedUri.query.afterLast;
	} else if (parsedUri.pathTail != NULL) {
		lastSegment = parsedUri.pathTail->text.afterLast;
	} else {
		//at least some path should be specified
		return false;
	}

	size_t pathLen = sizeof(char) * (lastSegment - parsedUri.hostText.afterLast);
	result->path = malloc(pathLen + 1);
	if (result->path == NULL) {
		return false;
	}
	strncpy(result->path, parsedUri.hostText.afterLast, pathLen);
	result->path[pathLen] = '\0';

	uriFreeUriMembersA(&parsedUri);
	return true;
}

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
	while (true) {
		free(line);
		line = NULL;
		if ((readBytes = getline(&line, &len, source)) == -1 || (strcmp(line, "\n") == 0)) {
			break;
		}
		char* uri = cutPrefix(line, "URI: ");
		if (uri != NULL) {
			result->uri = uri;
			if (!swift_uri_acquire_parse_uri(uri, result)) {
				break;
			}
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
	}

	free(line);
	line = NULL;

	if (result->uri == NULL) {
		fprintf(stderr, "invalid URIAcquire request. Expected URI\n");
		swift_uri_acquire_free(result);
		return NULL;
	}
	if (result->container == NULL || result->path == NULL) {
		fprintf(stderr, "invalid URIAcquire request. Invalid URI format. Expected: swift://container/path\n");
		swift_uri_acquire_free(result);
		return NULL;
	}
	if (result->filename == NULL) {
		fprintf(stderr, "invalid URIAcquire request. Expected Filename\n");
		swift_uri_acquire_free(result);
		return NULL;
	}
	return result;
}

void swift_uri_acquire_free(struct URIAcquire* obj) {
	if( obj == NULL ) {
		return;
	}
	free(obj->container);
	free(obj->filename);
	free(obj->lastModified);
	free(obj->path);
	free(obj->uri);
	free(obj);
}
