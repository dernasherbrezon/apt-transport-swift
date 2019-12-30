#ifndef COMMON_H_
#define COMMON_H_

#include <string.h>
#include <stdbool.h>

bool startsWith(const char *str, const char *pre);

char* cutPrefix(const char* str, const char* prefix);

char* trim(char *str);

char* concat(const char* str1, const char* str2);

#endif /* COMMON_H_ */
