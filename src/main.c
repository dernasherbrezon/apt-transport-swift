#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 201410L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <sys/types.h>

#include "common.h"
#include "Hashes.h"
#include "URIAcquire.h"
#include "Configuration.h"
#include "SwiftClient.h"

void swift_requestCapabilities() {
	fprintf(stdout, "100 Capabilities\n");
	fprintf(stdout, "Version: 1.2\n");
	fprintf(stdout, "Pipeline: false\n");
	fprintf(stdout, "Send-Config: true\n\n");
	fflush(stdout);
}

void swift_responseError(const char* uri, const char* error) {
	fprintf(stdout, "400 URI Failure\n");
	fprintf(stdout, "URI: %s\n", uri);
	fprintf(stdout, "Message: %s\n\n", error);
	fflush(stdout);
}

void swift_not_modified(const struct URIAcquire* request) {
	fprintf(stdout, "201 URI Done\n");
	fprintf(stdout, "URI: %s\n", request->uri);
	fprintf(stdout, "Filename: %s\n", request->filename);
	fprintf(stdout, "Last-Modified: \n");
	fprintf(stdout, "IMS-Hit: true\n\n");
	fflush(stdout);
}

void swift_responseStatus(const char* uri, const char* message) {
	fprintf(stdout, "102 Status\n");
	fprintf(stdout, "URI: %s\n", uri);
	fprintf(stdout, "Message: %s\n\n", message);
	fflush(stdout);
}

void swift_responseGeneralFailure(char *message) {
	fprintf(stdout, "401 General Failure\n");
	fprintf(stdout, "Message: %s\n\n", message);
	fflush(stdout);
}

void swift_response(const struct URIAcquire* request, const struct Hashes* hashes) {
	fprintf(stdout, "201 URI Done\n");
	fprintf(stdout, "URI: %s\n", request->uri);
	fprintf(stdout, "Filename: %s\n", request->filename);
	fprintf(stdout, "Size: %llu\n", ((unsigned long long) hashes->fileSize));
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
	fprintf(stdout, "Checksum-FileSize-Hash: %llu\n\n", ((unsigned long long) hashes->fileSize));
	fflush(stdout);
}

int main(void) {

	swift_requestCapabilities();

	struct SwiftClients *clients = NULL;
	struct Configuration *configuration = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t readBytes;

	while (true) {
		free(line);
		line = NULL;
		if ((readBytes = getline(&line, &len, stdin)) == -1) {
			break;
		}
		if (startsWith(line, "601")) {
			configuration = swift_configuration_read(stdin);
			if (configuration == NULL) {
				swift_responseGeneralFailure("unable to read configuration");
			}
			continue;
		}
		if (startsWith(line, "600")) {
			struct URIAcquire* message = swift_uri_acquire_read(stdin);
			if (message == NULL || configuration == NULL) {
				swift_responseGeneralFailure("unable to read URI acquire");
				continue;
			}
			if (clients == NULL) {
				curl_global_init(CURL_GLOBAL_DEFAULT);
			}
			struct SwiftClient *client = NULL;
			if (clients != NULL) {
				client = swift_client_find(&clients, message->container);
			}
			if (client == NULL) {
				swift_responseStatus(message->uri, "connecting...");
				client = swift_client_create(&clients, message->container, configuration);
				if (client == NULL) {
					swift_responseError(message->uri, "unable to connect");
					swift_uri_acquire_free(message);
					continue;
				}
				struct ContainerConfiguration *containerConfig = swift_configuration_find_by_container(configuration, message->container);
				if (containerConfig == NULL) {
					swift_responseError(message->uri, "unable to find configuration");
					swift_uri_acquire_free(message);
					continue;
				}
				swift_responseStatus(message->uri, "authenticating...");
				const char *errorResponse = swift_client_authenticate(client, containerConfig);
				if (errorResponse != NULL) {
					swift_responseError(message->uri, "unable to authenticate");
					swift_uri_acquire_free(message);
					continue;
				}
			} else if (client->token == NULL || client->endpointUrl == NULL) {
				swift_responseError(message->uri, "not authenticated");
				swift_uri_acquire_free(message);
				continue;
			}

			swift_responseStatus(message->uri, "downloading...");
			struct SwiftResponse *response = swift_client_download(client, message);
			if (response == NULL) {
				swift_responseError(message->uri, "unable to download");
				swift_uri_acquire_free(message);
				continue;
			}
			if (response->response_code == 304) {
				swift_not_modified(message);
				swift_uri_acquire_free(message);
				swift_client_response_free(response);
				continue;
			}
			if (response->response_code != 200) {
				swift_responseError(message->uri, response->response_message);
				swift_uri_acquire_free(message);
				swift_client_response_free(response);
				continue;
			}

			swift_client_response_free(response);

			FILE *pagefile = fopen(message->filename, "rb");
			struct Hashes* hashes = swift_hash_file(message, pagefile);
			fclose(pagefile);
			if (hashes == NULL) {
				swift_uri_acquire_free(message);
				continue;
			}
			swift_response(message, hashes);
			swift_uri_acquire_free(message);
			swift_hash_file_free(hashes);
			continue;
		}
	}
	free(line);
	line = NULL;

	swift_configuration_free(configuration);
	swift_client_clients_free(clients);
	curl_global_cleanup();

	return EXIT_SUCCESS;
}
