#include <string.h>
#include "strutil.h"

int endsWith(const char *str, const char *suffix) {
    int suffixLen = strlen(suffix);
    int len = strlen(str);
    if (0 == len || len < suffixLen) {
        return 0;
    }
    return memcmp(str + len - 1 - suffixLen, suffix, suffixLen);
}

int split(char *str, const char *splitter, char **vector, int max) {
    char *token;
    int n = 0;
    while ((token=strsep(&str, splitter)) && n < max) {
        vector[n++] = token;
    }
    return n;
}

