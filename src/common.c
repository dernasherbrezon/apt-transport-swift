#include <stdlib.h>
#include <ctype.h>

#include "common.h"

bool startsWith(const char *str, const char *pre) {
	return strncmp(pre, str, strlen(pre)) == 0;
}

char* cutPrefix(const char* str, const char* prefix) {
	size_t prefixLength = strlen(prefix);
	size_t strLength = strlen(str);
	if (prefixLength > strLength) {
		return NULL;
	}
	int i = 0;
	for (; i < prefixLength; i++) {
		if (str[i] != prefix[i]) {
			return NULL;
		}
	}
	size_t resultSize = strLength - i + 1;
	char* result = (char*) malloc(resultSize);
	strncpy(result, &str[i], resultSize);
	result[resultSize] = '\0';
	return trim(result);
}

char* substring(const char *str, size_t index) {
	size_t resultSize = strlen(str) - index;
	char* result = (char*) malloc(resultSize);
	strncpy(result, &str[index], strlen(str) - index);
	result[resultSize - 1] = '\0'; //-1 to replace \n
	return result;
}

char *trim(char *str) {
	size_t len = 0;
	char *frontp = str;
	char *endp = NULL;

	if (str == NULL) {
		return NULL;
	}
	if (str[0] == '\0') {
		return str;
	}

	len = strlen(str);
	endp = str + len;

	/* Move the front and back pointers to address the first non-whitespace
	 * characters from each end.
	 */
	while (isspace((unsigned char) *frontp)) {
		++frontp;
	}
	if (endp != frontp) {
		while (isspace((unsigned char) *(--endp)) && endp != frontp) {
		}
	}

	if (str + len - 1 != endp) {
		*(endp + 1) = '\0';
	} else if (frontp != str && endp == frontp) {
		*str = '\0';
	}

	/* Shift the string so that it starts at str so that if it's dynamically
	 * allocated, we can still free it on the returned pointer.  Note the reuse
	 * of endp to mean the front of the string buffer now.
	 */
	endp = str;
	if (frontp != str) {
		while (*frontp) {
			*endp++ = *frontp++;
		}
		*endp = '\0';
	}

	return str;
}
