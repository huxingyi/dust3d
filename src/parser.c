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
