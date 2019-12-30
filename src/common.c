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
	size_t resultSize = strLength - i;
	char* result = (char*) malloc(resultSize + 1);
	strncpy(result, &str[i], resultSize);
	result[resultSize] = '\0';
	return trim(result);
}

char *trim(char *str) {
	if (str == NULL) {
		return NULL;
	}
	if (str[0] == '\0') {
		return str;
	}

	char *end;

	// Trim leading space
	while (isspace((unsigned char) *str)) {
		str++;
	}

	if (*str == 0) {  // All spaces?
		return str;
	}

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char) *end)) {
		end--;
	}

	// Write new null terminator
	end[1] = '\0';

	return str;
}

char* concat(const char* str1, const char* str2) {
	char *result = malloc(strlen(str1) + strlen(str2) + 1);
	if( result == NULL ) {
		return NULL;
	}
	strncpy(result, str1, strlen(str1));
	result[strlen(str1)] = '\0';
	strncat(result, str2, strlen(str2));
	return result;
}
