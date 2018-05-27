#ifndef COMMON_H_
#define COMMON_H_

#include <string.h>
#include <stdbool.h>

bool startsWith(const char *str, const char *pre);

char* substring(const char *str, size_t index);

char* cutPrefix(const char* str, const char* prefix);

char *trim(char *str);

#endif /* COMMON_H_ */
