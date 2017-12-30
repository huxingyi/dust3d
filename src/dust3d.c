#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dust3d.h"
#include "rbtree.h"
#include "strutil.h"
#include "functionlist.h"

typedef struct registerNode {
    rbtreeNode node;
    char *name;
    dust3dFunction func;
} registerNode;

typedef struct meshNode {
    rbtreeNode node;
    char *name;
    mesh *mesh;
} meshNode;

struct dust3dState {
    rbtree registerMap;
    int argumentNum;
    int namingArgumentStart;
    char **arguments;
    mesh *currentMesh;
    rbtree meshMap;
    void *tag;
};

dust3dState *dust3dCreateState(void *tag) {
    dust3dState *state = (dust3dState *)calloc(1, sizeof(dust3dState));
    state->tag = tag;
    rbtreeInit(&state->registerMap, registerNode, node, name, rbtreeStringComparator);
    rust3dRegisterFunctionList(state);
    rbtreeInit(&state->meshMap, meshNode, node, name, rbtreeStringComparator);
    dust3dSetCurrentMesh(state, dust3dCreateMeshById(state, "default"));
    return state;
}

void *dust3dGetStateTag(dust3dState *state) {
    return state->tag;
}

void dust3dDestroyState(dust3dState *state) {
    registerNode *it = rbtreeFirst(&state->registerMap);
    while (it) {
        registerNode *del = it;
        it = rbtreeNext(&state->registerMap, it);
        free(del->name);
        free(del);
    }
    free(state);
}

int dust3dRegister(dust3dState *state, const char *name, dust3dFunction func) {
    registerNode *regNode = (registerNode *)calloc(1, sizeof(registerNode));
    regNode->name = strdup(name);
    regNode->func = func;
    rbtreeInsert(&state->registerMap, regNode);
    return 0;
}

static int callFunction(dust3dState *state, registerNode *regNode, int argumentNum, int namingArgumentStart, char **arguments) {
    int oldArgumentNum = state->argumentNum;
    int oldNamingArgumentStart = state->namingArgumentStart;
    char **oldArguments = state->arguments;
    int result;
    state->argumentNum = argumentNum;
    state->namingArgumentStart = namingArgumentStart;
    state->arguments = arguments;
    result = regNode->func(state);
    state->argumentNum = oldArgumentNum;
    state->namingArgumentStart = oldNamingArgumentStart;
    state->arguments = oldArguments;
    return result;
}

int dust3dRunOne(dust3dState *state, int argc, char *argv[]) {
    const char *name;
    registerNode *regNode;
    int argumentNum = 0;
    int namingArgumentStart = -1;
    int i = 0;
    const char *splitPos;
    int result;
    if ('-' != argv[0][0]) {
        fprintf(stderr, "Invalid parameter: \"%s\"\n", argv[0]);
        return -1;
    }
    name = &argv[0][1];
    regNode = rbtreeFind(&state->registerMap, &name);
    if (!regNode) {
        fprintf(stderr, "Unrecognized function name: \"%s\"\n", name);
        return -1;
    }
    for (i = 1; i < argc; i++) {
        if ('-' == argv[i][0]) {
            break;
        }
        splitPos = strchr(argv[i], ':');
        if (!splitPos) {
            if (-1 != namingArgumentStart) {
                fprintf(stderr, "Naming arguments should be at the end: \"%s\"\n", argv[i]);
                return -1;
            }
            argumentNum++;
            continue;
        }
        if (-1 == namingArgumentStart) {
            namingArgumentStart = argumentNum;
        }
        argumentNum++;
    }
    if (0 != (result=callFunction(state, regNode, argumentNum, namingArgumentStart, &argv[1]))) {
        fprintf(stderr, "Call failed with result: %d\n", result);
        return result;
    }
    return i;
}

int dust3dRun(dust3dState *state, int argc, char *argv[]) {
    int i = 0;
    int result = 0;
    while (i < argc) {
        result = dust3dRunOne(state, argc - i, &argv[i]);
        if (result < 0) {
            return result;
        }
        if (0 == result) {
            break;
        }
        i += result;
    }
    return i;
}

const char *dust3dGetNthArgument(dust3dState *state, int n) {
    if (n >= state->argumentNum) {
        return 0;
    }
    if (-1 != state->namingArgumentStart) {
        if (n >= state->namingArgumentStart) {
            return 0;
        }
    }
    return state->arguments[n];
}

const char *dust3dGetNamingArgument(dust3dState *state, const char *name) {
    int i;
    int nameLen;
    if (-1 == state->namingArgumentStart) {
        return 0;
    }
    nameLen = (int)strlen(name);
    for (i = state->namingArgumentStart; i < state->argumentNum; i++) {
        const char *p = state->arguments[i];
        const char *pos = strstr(p, name); // swizzling
        const char *splitter;
        if (!pos) {
            continue;
        }
        splitter = strchr(p, ':');
        if (!splitter) {
            continue;
        }
        if (pos + nameLen > splitter) {
            continue;
        }
        return &splitter[1];
    }
    return 0;
}

static meshNode *findMeshNode(dust3dState *state, const char *id) {
    return (meshNode *)rbtreeFind(&state->meshMap, &id);
}

static meshNode *newMeshNode(dust3dState *state, const char *id) {
    meshNode *node = (meshNode *)calloc(1, sizeof(meshNode));
    node->name = strdup(id);
    rbtreeInsert(&state->meshMap, node);
    return node;
}

static void deleteMeshNode(meshNode *node) {
    free(node->name);
    free(node);
}

mesh *dust3dFindMeshById(dust3dState *state, const char *id) {
    meshNode *node = findMeshNode(state, id);
    return node ? node->mesh : 0;
}

mesh *dust3dCreateMeshById(dust3dState *state, const char *id) {
    meshNode *node = newMeshNode(state, id);
    node->mesh = halfedgeCreateMesh();
    return node->mesh;
}

void dust3dDestroyMeshById(dust3dState *state, const char *id) {
    meshNode *node = findMeshNode(state, id);
    deleteMeshNode(node);
}

mesh *dust3dGetCurrentMesh(dust3dState *state) {
    return state->currentMesh;
}

mesh *dust3dSetCurrentMesh(dust3dState *state, mesh *m) {
    mesh *last = state->currentMesh;
    state->currentMesh = m;
    return last;
}

vector3d toVector3d(const char *str) {
    vector3d v;
    char *a = strdup(str);
    char *vector[3];
    int n = split(a, ",", vector, 3);
    if (3 != n) {
        fprintf(stderr, "Invalid format:\"%s\"\n", str);
        exit(-1);
    }
    v.x = atof(vector[0]);
    v.y = atof(vector[1]);
    v.z = atof(vector[2]);
    free(a);
    return v;
}

float toFloat(const char *str) {
    return atof(str);
}

float dust3dGetNamingFloat(dust3dState *state, const char *name, float def) {
    const char *str = dust3dGetNamingArgument(state, name);
    if (!str) {
        return def;
    }
    return toFloat(str);
}

int dust3dGetNamingInt(dust3dState *state, const char *name, int def) {
    const char *str = dust3dGetNamingArgument(state, name);
    if (!str) {
        return def;
    }
    return atoi(str);
}


