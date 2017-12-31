#include "dust3d.h"

char *g_testArgv[] = {
    "-add", "cube", "w:1", "d:0.7", "h:0.5,",
    "-scale", "f:0", "e:0", "a:1.2",
    "-scale", "f:4", "e:0", "a:1.1",
    "-show"
};

int main(int argc, char *argv[]) {
    dust3dState *state = dust3dCreateState(0);
    return dust3dRun(state, sizeof(g_testArgv) / sizeof(g_testArgv[0]), g_testArgv);
}

