#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "halfedge.h"
#include "rbtree.h"
#include "parser.h"

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
            float radius = parserFindFloatOptionOr(&p, "radius", PARSER_DEFAULT_RADIUS);
            if (p.verbose) {
                printf("\tbase mesh type:\"%s\"\n", base);
                printf("\tradius:%f\n", radius);
            }
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
            float amount = parserFindFloatOptionOr(&p, "amount", 0.1);
            if (isdigit(index[0])) {
                vertexIndex = atoi(index);
            }
            if (p.verbose) {
                printf("\tvertex index:%d\n", vertexIndex);
                printf("\tamount:%f\n", amount);
            }
            halfedgeChamferVertex(p.m, parserMeshGetVertexByIndexOrExit(p.m, vertexIndex), amount);
        }
    }
    if (p.verbose) {
        printf("Saving to \"%s\"..\n", PARSER_DEFAULT_OUTPUT_OBJ_NAME);
    }
    halfedgeSaveMeshAsObj(p.m, PARSER_DEFAULT_OUTPUT_OBJ_NAME);
    return 0;
}
