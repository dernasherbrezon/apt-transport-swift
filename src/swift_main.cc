#include <apt-pkg/fileutl.h>
#include <sys/signal.h>
#include <iostream>
#include <string>
#include <type_traits>

#include "SwiftMethod.h"

int main(int, const char *argv[]) {
	// ignore SIGPIPE, this can happen on write() if the socket
	// closes the connection (this is dealt with via ServerDie())
	signal(SIGPIPE, SIG_IGN);
	std::string Binary = flNotDir(argv[0]);
	return SwiftMethod(std::move(Binary)).Loop();
}

