#include "dust3d.h"

char *g_testArgv[] = {
    "-add",
        "/Users/jeremy/Repositories/dust3d/gourd.obj",
    /*"-add",
        "cube",
        "r:1.2",*/
    "-move",
        "x:0.2",
        "y:0.5",
        "z:0.6",
    "-new",
        "box",
    "-sel",
        "box",
    "-add",
        "cube",
        "r:0.8",
    "-inter",
        "default",
        "box",
    "-show"
};

int main(int argc, char *argv[]) {
    dust3dState *state = dust3dCreateState(0);
    return dust3dRun(state, sizeof(g_testArgv) / sizeof(g_testArgv[0]), g_testArgv);
}
