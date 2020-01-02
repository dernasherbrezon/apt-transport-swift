#ifndef HASHES_H_
#define HASHES_H_

#include <stdio.h>
#include <sys/types.h>

#include "URIAcquire.h"

struct Hashes {
	char* md5;
	char* sha1;
	char* sha256;
	char* sha512;
	off_t fileSize;
};

struct Hashes* swift_hash_file(FILE* file);

void swift_hash_file_free(struct Hashes* obj);

#endif /* HASHES_H_ */
