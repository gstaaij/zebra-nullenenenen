#define NOB_IMPLEMENTATION
#include "src/nob.h"


int main(int argc, char* argv[]) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    nob_mkdir_if_not_exists("./build");

    Nob_Cmd cmd = {0};
    // Compile for Windows
    cmd.count = 0;
    nob_cmd_append(&cmd, "x86_64-w64-mingw32-gcc", "-Wall", "-Wextra");
    nob_cmd_append(&cmd, "-o", "build/main");
    nob_cmd_append(&cmd, "src/main.c");
    nob_cmd_append(&cmd, "-lm");
    if (!nob_cmd_run_sync(cmd)) return 1;

#ifndef _WIN32
    // Compile for Linux
    cmd.count = 0;
    nob_cmd_append(&cmd, "gcc", "-Wall", "-Wextra");
    nob_cmd_append(&cmd, "-o", "build/main");
    nob_cmd_append(&cmd, "src/main.c");
    nob_cmd_append(&cmd, "-lm");
    if (!nob_cmd_run_sync(cmd)) return 1;
#endif // _WIN32

    nob_cmd_free(cmd);
    return 0;
}
