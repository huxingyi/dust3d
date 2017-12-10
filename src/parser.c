#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "parser.h"

void parserInit(parser *p, int argc, char *argv[]) {
    memset(p, 0, sizeof(parser));
    p->argc = argc;
    p->argv = argv;
    p->current = 1;
}

const char *parserGetNextItem(parser *p, char type) {
    if (p->current >= p->argc) {
        return 0;
    }
    if (type) {
        if (type != p->argv[p->current][0]) {
            return 0;
        }
        return &p->argv[p->current++][1];
    }
    return &p->argv[p->current++][0];
}

const char *parserGetNextCommand(parser *p) {
    return parserGetNextItem(p, '-');
}

const char *parserGetNextCommandSkipOthers(parser *p) {
    while (p->current < p->argc) {
        if ('-' == p->argv[p->current][0]) {
            return &p->argv[p->current++][1];
        }
        p->current++;
    }
    return 0;
}

const char *parserFindOption(parser *p, const char *optionName) {
    const char *result = 0;
    int position = p->current;
    while (p->current < p->argc) {
        if ('+' == p->argv[p->current][0]) {
            if (0 == strcmp(&p->argv[p->current][1], optionName)) {
                p->current++;
                result = parserGetNextValueOrExit(p);
                break;
            }
        } else if ('-' == p->argv[p->current][0]) {
            break;
        }
        p->current++;
    }
    p->current = position;
    return result;
}

const char *parserFindOptionOr(parser *p, const char *optionName, const char *orValue) {
    const char *result = parserFindOption(p, optionName);
    if (result) {
        return result;
    }
    return orValue;
}

float parserFindFloatOptionOr(parser *p, const char *optionName, float orValue) {
    const char *result = parserFindOption(p, optionName);
    if (result) {
        return atof(result);
    }
    return orValue;
}

int parserFindIntOptionOr(parser *p, const char *optionName, int orValue) {
    const char *result = parserFindOption(p, optionName);
    if (result) {
        return atoi(result);
    }
    return orValue;
}

static int split(char *str, const char *splitter, char **vector, int max) {
    char *token;
    int n = 0;
    while ((token=strsep(&str, splitter)) && n < max) {
        vector[n++] = token;
    }
    return n;
}

static vector3d convertStringToVector3dOrExit(const char *str) {
    vector3d v;
    char *a = strdup(str);
    char *vector[3];
    int n = split(a, ",", vector, 3);
    if (3 != n) {
        printf("Invalid format:\"%s\"\n", str);
        exit(-1);
    }
    v.x = atof(vector[0]);
    v.y = atof(vector[1]);
    v.z = atof(vector[2]);
    free(a);
    return v;
}

vector3d parserFindVector3dOptionOr(parser *p, const char *optionName, vector3d orValue) {
    const char *result = parserFindOption(p, optionName);
    if (result) {
        return convertStringToVector3dOrExit(result);
    }
    return orValue;
}

const char *parserGetNextValue(parser *p) {
    return parserGetNextItem(p, '\0');
}

const char *parserGetNextValueOrExit(parser *p) {
    const char *result = parserGetNextValue(p);
    if (!result) {
        printf("Expect value but got none\n");
        exit(-1);
    }
    return result;
}

vertex *parserMeshGetVertexByIndex(mesh *m, int index) {
    vertex *v = m->firstVertex;
    while (v && index > 0) {
        v = v->next;
        index--;
    }
    if (0 != index) {
        return 0;
    }
    return v;
}

vertex *parserMeshGetVertexByIndexOrExit(mesh *m, int index) {
    vertex *result = parserMeshGetVertexByIndex(m, index);
    if (!result) {
        printf("Get vertex by index:%d from mesh failed\n", index);
        exit(-1);
    }
    return result;
}
