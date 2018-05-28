#ifndef URIACQUIRE_H_
#define URIACQUIRE_H_

#include <stdbool.h>
#include <stdio.h>

struct URIAcquire {
	char *uri;
	char *filename;
	bool expectedMd5;
	bool expectedSha1;
	bool expectedSha256;
	bool expectedSha512;
	char *lastModified;
	char *container;
	char *username;
};

struct URIAcquire* swift_uri_acquire_read(FILE *source);

#endif /* URIACQUIRE_H_ */
