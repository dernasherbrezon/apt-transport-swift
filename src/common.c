#include <stdlib.h>

#include "common.h"

bool startsWith(const char *str, const char *pre) {
	return strncmp(pre, str, strlen(pre)) == 0;
}

char* substring(const char *str, size_t index) {
	size_t resultSize = strlen(str) - index;
	char* result = (char*) malloc(resultSize);
	strncpy(result, &str[index], strlen(str) - index);
	result[resultSize - 1] = '\0'; //-1 to replace \n
	return result;
}
