#ifndef HASHES_H_
#define HASHES_H_

#include <stdio.h>

#include "URIAcquire.h"

struct Hashes {
	char* md5;
	char* sha1;
	char* sha256;
	char* sha512;
	off_t fileSize;
};

struct Hashes* swift_hash_file(struct URIAcquire* req, FILE* file);

#endif /* HASHES_H_ */
