#ifndef DUST3D_PARSER_H
#define DUST3D_PARSER_H
#include "halfedge.h"

#define PARSER_DEFAULT_RADIUS               1.0
#define PARSER_DEFAULT_OUTPUT_OBJ_NAME      "dust3d.obj"

typedef struct parser {
    int argc;
    char **argv;
    int current;
    int verbose;
    mesh *m;
} parser;

void parserInit(parser *p, int argc, char *argv[]);
const char *parserGetNextItem(parser *p, char type);
const char *parserGetNextCommand(parser *p);
const char *parserGetNextCommandSkipOthers(parser *p);
const char *parserFindOption(parser *p, const char *optionName);
const char *parserFindOptionOr(parser *p, const char *optionName, const char *orValue);
float parserFindFloatOptionOr(parser *p, const char *optionName, float orValue);
int parserFindIntOptionOr(parser *p, const char *optionName, int orValue);
vector3d parserFindVector3dOptionOr(parser *p, const char *optionName, vector3d orValue);
#define parserFindPoint3dOptionOr(p, optionName, orValue)   parserFindVector3dOptionOr(p, optionName, orValue)
const char *parserGetNextValue(parser *p);
const char *parserGetNextValueOrExit(parser *p);
vertex *parserMeshGetVertexByIndex(mesh *m, int index);
vertex *parserMeshGetVertexByIndexOrExit(mesh *m, int index);

#endif
