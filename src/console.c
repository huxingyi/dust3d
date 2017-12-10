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
                printf("\tUnrecognized base mesh type:\"%s\"\n", base);
                exit(-1);
            }
        }
        else if (0 == strcmp("chamfer-by-vertex-index", cmd)) {
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
        } else if (0 == strcmp("chamfer-by-plane", cmd)) {
            vector3d defaultNormal = {0, 0, 0};
            point3d defaultPoint = {0, 0, 0};
            vector3d normal = parserFindVector3dOptionOr(&p, "normal", defaultNormal);
            point3d point = parserFindPoint3dOptionOr(&p, "origin", defaultPoint);
            mesh *frontMesh;
            if (p.verbose) {
                printf("\tnormal:(%f,%f,%f)\n", normal.x, normal.y, normal.z);
                printf("\torigin:(%f,%f,%f)\n", point.x, point.y, point.z);
            }
            frontMesh = halfedgeSliceMeshByFace(p.m, &point, &normal);
            halfedgeDestroyMesh(frontMesh);
        } else {
            printf("\tUnrecognized command:\"%s\"\n", cmd);
            exit(-1);
        }
    }
    if (p.verbose) {
        printf("Saving to \"%s\"..\n", PARSER_DEFAULT_OUTPUT_OBJ_NAME);
    }
    halfedgeSaveMeshAsObj(p.m, PARSER_DEFAULT_OUTPUT_OBJ_NAME);
    return 0;
}
