#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "halfedge.h"
#include "rbtree.h"

#define DUST3D_DEFAULT_RADIUS               1.0
#define DUST3D_DEFAULT_OUTPUT_OBJ_NAME      "dust3d.obj"

typedef struct parser {
    int argc;
    char **argv;
    int current;
    int verbose;
    mesh *m;
} parser;

static const char *parserGetNextCommand(parser *p);

static void parserInit(parser *p, int argc, char *argv[]) {
    memset(p, 0, sizeof(parser));
    p->argc = argc;
    p->argv = argv;
    p->current = 1;
}

static const char *parserGetNextItem(parser *p, char type) {
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

static const char *parserGetNextCommand(parser *p) {
    return parserGetNextItem(p, '-');
}

static const char *parserGetNextCommandSkipOthers(parser *p) {
    while (p->current < p->argc) {
        if ('-' == p->argv[p->current][0]) {
            return &p->argv[p->current++][1];
        }
        p->current++;
    }
    return 0;
}

static const char *parserGetNextOptionName(parser *p) {
    return parserGetNextItem(p, '+');
}

static const char *parserGetNextAtName(parser *p) {
    return parserGetNextItem(p, '@');
}

static const char *parserGetNextValue(parser *p) {
    return parserGetNextItem(p, '\0');
}

static const char *parserGetNextValueOrExit(parser *p) {
    const char *result = parserGetNextValue(p);
    if (!result) {
        printf("Expect value but got none\n");
        exit(-1);
    }
    return result;
}

static vertex *meshGetVertexByIndex(mesh *m, int index) {
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

static vertex *meshGetVertexByIndexOrExit(mesh *m, int index) {
    vertex *result = meshGetVertexByIndex(m, index);
    if (!result) {
        printf("Get vertex by index:%d from mesh failed\n", index);
        exit(-1);
    }
    return result;
}

int main(int argc, char *argv[]) {
    parser p;
    const char *cmd;
    parserInit(&p, argc, argv);
    p.verbose = 1;
    p.m = halfedgeCreateMesh();
    for (cmd=parserGetNextCommand(&p); cmd; cmd=parserGetNextCommandSkipOthers(&p)) {
        if (p.verbose) {
            printf("Processing command:\"%s\"..\n", cmd);
        }
        if (0 == strcmp("add", cmd)) {
            const char *base = parserGetNextValueOrExit(&p);
            float radius = DUST3D_DEFAULT_RADIUS;
            if (p.verbose) {
                printf("\tbase mesh type:\"%s\"\n", base);
                printf("\tradius:%f\n", radius);
            }
            //TODO: parse options
            if (0 == strcmp("cube", base)) {
                halfedgeExtrudeFace(p.m, halfedgeCreatePlane(p.m, radius), radius);
            } else {
                printf("Unrecognized base mesh type:\"%s\"\n", base);
                exit(-1);
            }
        }
        else if (0 == strcmp("chamfer", cmd)) {
            const char *index = parserGetNextValueOrExit(&p);
            int vertexIndex = 0;
            float amount = 0.1;
            if (isdigit(index[0])) {
                vertexIndex = atoi(index);
            }
            if (p.verbose) {
                printf("\tvertex index:%d\n", vertexIndex);
                printf("\tamount:%f\n", amount);
            }
            halfedgeChamferVertex(p.m, meshGetVertexByIndexOrExit(p.m, vertexIndex), amount);
        }
    }
    if (p.verbose) {
        printf("Saving to \"%s\"..\n", DUST3D_DEFAULT_OUTPUT_OBJ_NAME);
    }
    halfedgeSaveMeshAsObj(p.m, DUST3D_DEFAULT_OUTPUT_OBJ_NAME);
    return 0;
}
