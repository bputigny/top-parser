#include "FrontEnd.h"
#include "Analysis.h"

#include <fstream>

int main(int argc, char *argv[]) {
    std::ofstream f;
    FrontEnd fe;
    ir::Program *p = NULL;

    if (argc == 2) {
        p = fe.parse(argv[1]);
    }
    else {
        p = fe.parse("test.edl");
    }

    return 0;
}
