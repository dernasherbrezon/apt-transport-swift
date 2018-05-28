#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

struct Configuration {
	char *proxyHostPort;
	char *container;
	char *username;
	char *password;
};

struct Configuration* swift_configuration_read();

#endif /* CONFIGURATION_H_ */
