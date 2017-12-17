#include "dust3d.h"

int main(int argc, char *argv[]) {
    dust3dState *state = dust3dCreateState(0);
    dust3dRun(state, argc - 1, argv + 1);
    return 0;
}
