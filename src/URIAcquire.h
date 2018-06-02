#ifndef URIACQUIRE_H_
#define URIACQUIRE_H_

#include <stdbool.h>
#include <stdio.h>

struct URIAcquire {
	char *uri;
	char *container;
	char *path;

	char *filename;
	bool expectedMd5;
	bool expectedSha1;
	bool expectedSha256;
	bool expectedSha512;
	char *lastModified;
};

struct URIAcquire* swift_uri_acquire_read(FILE *source);

void swift_uri_acquire_free(struct URIAcquire* obj);

#endif /* URIACQUIRE_H_ */
