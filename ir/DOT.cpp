#include "config.h"
#include "Printer.h"
#include "DOT.h"

void DOT::display(const std::string& file) {
#ifdef HAVE_DOT
    std::string cmd = "dot -Txlib " + file + " &";
    system(cmd.c_str());
#else
    err() << "cannot disply graph: graphviz not found\n";
#endif
}


void DOT::display() {
    char *n = tmpnam(NULL);
    std::string tmp(n);
    std::ofstream f;
    f.open(tmp);
    this->dumpDOT(f, true);
    f.close();
    DOT::display(tmp);
}
