#include "dust3d.h"

char *g_testArgv[] = {
    "-add",
        "/Users/jeremy/Repositories/dust3d/teapot.obj",
    "-show"
};

int main(int argc, char *argv[]) {
    dust3dState *state = dust3dCreateState(0);
    return dust3dRun(state, sizeof(g_testArgv) / sizeof(g_testArgv[0]), g_testArgv);
}
