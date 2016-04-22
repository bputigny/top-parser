#include "FrontEnd.h"
#include "Analysis.h"

#include <fstream>

int main(int argc, char *argv[]) {
    std::ofstream f;
    FrontEnd fe;

    if (argc == 2) {
        fe.parse(argv[1]);
    }
    else {
        fe.parse(std::string("test.edl"));
    }

    return 0;
}
