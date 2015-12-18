#include "config.h"
#include "Printer.h"
#include "DOT.h"

void DOT::displayFile(const std::string& file) {
#ifdef HAVE_DOT
    std::string cmd = "dot -Txlib " + file + " &";
    system(cmd.c_str());
#else
    err() << "cannot disply graph in `" << file << "': graphviz (dot) not found\n";
#endif
}


void DOT::display(std::string title) {
    char *n = tmpnam(NULL);
    std::string tmp(n);
    std::ofstream f;
    f.open(tmp);
    this->dumpDOT(f, title, true);
    f.close();
    DOT::displayFile(tmp);
}
