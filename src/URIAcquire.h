#ifndef URIACQUIRE_H_
#define URIACQUIRE_H_

#include <stdbool.h>

#include "common.h"

struct URIAcquire {
	char *uri;
	char *filename;
	bool expectedMd5;
	bool expectedSha1;
	bool expectedSha256;
	bool expectedSha512;
//FIXME last modified?
};

struct URIAcquire* swift_uri_acquire_read();

#endif /* URIACQUIRE_H_ */
