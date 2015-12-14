#include "FrontEnd.h"

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

    f.open("test.dot");
    p->dumpDOT(f);
    p->buildSymTab();
    f.close();

    f.open("symtab.dot");
    p->getSymTab()->dumpDOT(f);
    f.close();

    return 0;
}
