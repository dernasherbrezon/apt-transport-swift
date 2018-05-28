#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdio.h>

struct Configuration {
	char *proxyHostPort;
	char *container;
	char *username;
	char *password;
};

struct Configuration* swift_configuration_read(FILE* source);

#endif /* CONFIGURATION_H_ */
