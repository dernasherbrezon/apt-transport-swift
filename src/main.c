/*
 ============================================================================
 Name        : apt-transport-swift2.c
 Author      : dernasherbrezon
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <curl/curl.h>

#include "common.h"
#include "Hashes.h"
#include "URIAcquire.h"
#include "Configuration.h"

void swift_requestCapabilities() {
	fprintf(stdout, "100 Capabilities\n");
	fprintf(stdout, "Version: 1.2\n");
	fprintf(stdout, "Pipeline: false\n");
	fprintf(stdout, "Send-Config: true\n");
}

void swift_responseError(const char* uri, const char* error) {
	fprintf(stdout, "400 URI Failure\n");
	fprintf(stdout, "URI: %s\n", uri);
	fprintf(stdout, "Message: %s\n", error);
}

void swift_not_modified(const struct URIAcquire* request) {
	fprintf(stdout, "201 URI Done\n");
	fprintf(stdout, "URI: %s\n", request->uri);
	fprintf(stdout, "Filename: %s\n", request->filename);
	fprintf(stdout, "Last-Modified: \n");
	fprintf(stdout, "IMS-Hit: true\n");
}

void swift_responseStatus(const char* uri, const char* message) {
	fprintf(stdout, "102 Status\n");
	fprintf(stdout, "URI: %s\n", uri);
	fprintf(stdout, "Message: %s\n", message);
}

void swift_response(const struct URIAcquire* request,
		const struct Hashes* hashes) {
	fprintf(stdout, "201 URI Done\n");
	fprintf(stdout, "URI: %s\n", request->uri);
	fprintf(stdout, "Filename: %s\n", request->filename);
	fprintf(stdout, "Size: %llu\n", hashes->fileSize);
	if (hashes->md5 != NULL) {
		fprintf(stdout, "MD5-Hash: %s\n", hashes->md5);
		fprintf(stdout, "MD5Sum-Hash: %s\n", hashes->md5);
	}
	if (hashes->sha1 != NULL) {
		fprintf(stdout, "SHA1-Hash: %s\n", hashes->sha1);
	}
	if (hashes->sha256 != NULL) {
		fprintf(stdout, "SHA256-Hash: %s\n", hashes->sha256);
	}
	if (hashes->sha512 != NULL) {
		fprintf(stdout, "SHA512-Hash: %s\n", hashes->sha512);
	}
	fprintf(stdout, "Checksum-FileSize-Hash: %llu\n", hashes->fileSize);
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
	return written;
}

int main(void) {

	CURL *curl = NULL;

	swift_requestCapabilities();

	char *line = NULL;
	size_t len = 0;
	ssize_t readBytes;

	struct Configuration *configuration = NULL;

	while (true) {
		if ((readBytes = getline(&line, &len, stdin)) == -1) {
			break;
		}
		if (startsWith(line, "601")) {
			configuration = swift_configuration_read();
			if (configuration == NULL) {
				continue;
			}
			//FIXME handle configuration
		} else if (startsWith(line, "600")) {
			struct URIAcquire* message = swift_uri_acquire_read();
			if (message == NULL || configuration == NULL) {
				continue;
			}
			if (curl == NULL) {
				curl_global_init(CURL_GLOBAL_DEFAULT);
				curl = curl_easy_init();
				if (!curl) {
					break;
				}
			}

			curl_easy_setopt(curl, CURLOPT_URL, message->uri);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			FILE *pagefile = fopen(message->filename, "wb");
			if (!pagefile) {
				swift_responseError(message->uri, "unable to write file");
				continue;
			}
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
			CURLcode res = curl_easy_perform(curl);
			fclose(pagefile);
			if (res != CURLE_OK) {
				swift_responseError(message->uri, curl_easy_strerror(res));
				continue;
			}
			long response_code;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code == 304) {
				swift_not_modified(message);
				continue;
			}
			if (response_code != 200) {
				swift_responseError(message->uri, "Not good");
				continue;
			}

			pagefile = fopen(message->filename, "rb");
			struct Hashes* hashes = swift_hash_file(message, pagefile);
			if (hashes == NULL) {
				continue;
			}
			fclose(pagefile);
			swift_response(message, hashes);
			free(message);
			free(hashes);
		}
	}

	free(configuration);

	curl_easy_cleanup(curl);
	return EXIT_SUCCESS;
}
