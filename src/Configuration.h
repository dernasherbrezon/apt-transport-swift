#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdio.h>
#include <stdbool.h>

struct ContainerConfiguration {
	char *id;
	char *container;
	char *username;
	char *password;
	char *url;
};

struct ListOfConfigurations {
	size_t count;
	struct ContainerConfiguration* value[];
};

struct Configuration {
	char *proxyHostPort;
	struct ListOfConfigurations *containers;
	bool verbose;
};

struct Configuration* swift_configuration_read(FILE* source);

struct ContainerConfiguration* swift_configuration_find_by_container(struct Configuration *config, const char* containerName);

#endif /* CONFIGURATION_H_ */
