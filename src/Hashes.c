#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 201410L
#endif
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "Hashes.h"

char* swift_convertToHexString(unsigned char* str, int arraySize) {
	char* result = malloc(sizeof(char) * (arraySize * 2 + 1));
	for (int i = 0; i < arraySize; i++) {
		sprintf(&result[i * 2], "%02x", str[i]);
	}
	result[arraySize * 2] = '\0';
	return result;
}

struct Hashes* swift_hash_file(struct URIAcquire* req, FILE* file) {
	if( file == NULL ) {
		return NULL;
	}
	struct Hashes* result = malloc(sizeof(struct Hashes));
	if (result == NULL) {
		return NULL;
	}
	MD5_CTX mdContext;
	if (req->expectedMd5) {
		MD5_Init(&mdContext);
	}

	SHA_CTX sha1Context;
	if (req->expectedSha1) {
		SHA1_Init(&sha1Context);
	}

	SHA256_CTX sha256Context;
	if (req->expectedSha256) {
		SHA256_Init(&sha256Context);
	}

	SHA512_CTX sha512Context;
	if (req->expectedSha512) {
		SHA512_Init(&sha512Context);
	}

	int bytes;
	unsigned char data[1024];
	while ((bytes = fread(data, 1, 1024, file)) != 0) {
		if (req->expectedMd5) {
			MD5_Update(&mdContext, data, bytes);
		}
		if (req->expectedSha1) {
			SHA1_Update(&sha1Context, data, bytes);
		}
		if (req->expectedSha256) {
			SHA256_Update(&sha256Context, data, bytes);
		}
		if (req->expectedSha512) {
			SHA512_Update(&sha512Context, data, bytes);
		}
	}
	if (req->expectedMd5) {
		unsigned char* md5 = malloc(sizeof(char) * MD5_DIGEST_LENGTH);
		MD5_Final(md5, &mdContext);
		result->md5 = swift_convertToHexString(md5, MD5_DIGEST_LENGTH);
		free(md5);
	} else {
		result->md5 = NULL;
	}
	if (req->expectedSha1) {
		unsigned char* sha1 = malloc(sizeof(char) * SHA_DIGEST_LENGTH);
		SHA1_Final(sha1, &sha1Context);
		result->sha1 = swift_convertToHexString(sha1, SHA_DIGEST_LENGTH);
		free(sha1);
	} else {
		result->sha1 = NULL;
	}
	if (req->expectedSha256) {
		unsigned char* sha256 = malloc(sizeof(char) * SHA256_DIGEST_LENGTH);
		SHA256_Final(sha256, &sha256Context);
		result->sha256 = swift_convertToHexString(sha256, SHA256_DIGEST_LENGTH);
		free(sha256);
	} else {
		result->sha256 = NULL;
	}
	if (req->expectedSha512) {
		unsigned char* sha512 = malloc(sizeof(char) * SHA512_DIGEST_LENGTH);
		SHA512_Final(sha512, &sha512Context);
		result->sha512 = swift_convertToHexString(sha512, SHA512_DIGEST_LENGTH);
		free(sha512);
	} else {
		result->sha512 = NULL;
	}
	struct stat buf;
	if (fstat(fileno(file), &buf) == 0) {
		result->fileSize = buf.st_size;
	}
	return result;
}

void swift_hash_file_free(struct Hashes* obj) {
	if( obj == NULL ) {
		return;
	}
	free(obj->md5);
	free(obj->sha1);
	free(obj->sha256);
	free(obj->sha512);
	free(obj);
}
